// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double fSigcheckVerificationFactor = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64 nTimeLastCheckpoint;
        int64 nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (  0, uint256("0xb19f7a9a051ad5a1ad5f214cdd71bdf1e93978934e653d32e31164d5b47b7bab"))
        (  1, uint256("0x7a7b6ec671f4af413bc3a382a305ba091794621901fc4827268abb71c39e958a"))
        (  100, uint256("0x6e9945ca952154697d58ee149b93a43f539e0caf9250f70b972d3723693e2613"))
        (  500, uint256("0x52e7ccaf779190dcbc560615b54fa364c6afcb92c8cdf760e6e2189d5586485f"))
        (  1000, uint256("0xa5f9dcb9abb1850250fb48760bb0fc85e7622164ad7ffceff8cb25ad02a74fc7"))
        (  5000, uint256("0x6adb09c0847ff889e03a4e00d4d52adce5596189cf2589e570fa935b871c346b"))
        (  10000, uint256("0x8c5fe90a360ed502013452fc06be1fc91b763decb893b4d6320f13a11e33039b"))
        (  25000, uint256("0xc26dffad01f47d46ba3312e8844843ea8ac2c7068c7395b9ac4fa5107b1ee63f"))
        (  50000, uint256("0xe6d8c9bc2d386dc246c64eefd8cd5aff9ae3ed13c6718e447b1dc79662d19b44"))
        (  100000, uint256("0xbebfb5a5cfe6b35e9e83570d9736f0c057e9674d08bff99ab89537692eff9d34"))
        (  250000, uint256("0x3e76e2cb07d21f8b5277f394f3925e4b2dc4f4683f4c1cf04768f5c1eeed8288"))
        (  500000, uint256("0x36d79bba702a20cc9d308b65ba058190d0b1c10df50c0227ece416ec73a05473"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1613520897, // * UNIX timestamp of last checkpoint block
        0,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        1.0     // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        (   0, uint256("0x634af3921af234a34edacc85b951db4b2a9010df7362e9a6ee1c97d0b8f60e1f"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1613520858,
        0,
        1.0
    };

    const CCheckpointData &Checkpoints() {
        if (fTestNet)
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!GetBoolArg("-checkpoints", true))
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex) {
        if (pindex==NULL)
            return 0.0;

        int64 nNow = time(NULL);

        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!GetBoolArg("-checkpoints", true))
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
