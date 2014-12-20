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

    bool fEnabled = true;

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        ( 0,     uint256("0x00000447468db4b26ca942c2c6c700b49907e9d0d8ca6ce2bd84223a74972259"))
        ( 5000,     uint256("0x00000000001070b052725af8bf364b9442f957347aa5344304c5c031dd373253"))
        ( 15000,     uint256("0x000000000003d26b8091f8220f151218bb56fcb648ca70bb92968b83ac469848"))
        ( 25000,     uint256("0x000000000004b042f2f61bea4a20d5e8823361d9615e352e34c8ea9b66b3a955"))
        ( 46000,     uint256("0x000000000003f239dcabca52a88f491a07da7e5964d6c7ee9eedbf0f0e44d0e5"))
        ( 75000,     uint256("0x000000000000d00a287b580ababb3021824188af3c20de707b715b9e1a00a11b"))
        ( 130000,     uint256("0x0000000000076471fa8f9657e9b36ab5c0e4d8aeb6521f6fc4c3792cca7cd8ed"))
        ( 152000,     uint256("0x000000000016394b208819eb640771ff7f01e8e7d6dfffba3b11c10a486f57bd"))
        ( 177682,     uint256("0x0000000000003319bcbad75ba0ead5f82b2783ae385b8063a81d86f6880338a6"))
        ( 188004,     uint256("0x000000000000392893fcf5e9360f09123a395f3f00c72023971a12cc9619d325"))
        ( 200006,     uint256("0x00000000000434eb6b1b3c22daa0230eae4cdeded7d62794fdb695b3f034298a"))
        ( 218000,     uint256("0x000000000006122654c1a462af145c4f0f54ebbc4126bdff5fe07cd1ccafa071"))
        ( 235000,     uint256("0x000000000000cbac1b0fd62d9d21402f57d7fc4554da3415c964cfe072ad7539"))
        ( 239143,     uint256("0x000000000099e3b5fdf2f2c4906eb0c9ea7e4183dbc30c66ce2b682efa8081be"))
        
        ;
        
    static const CCheckpointData data = {
        &mapCheckpoints,
        1410084058, // * UNIX timestamp of first checkpoint block
        1,     // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        2880        // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, uint256("0x"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1374901773,
        0,
        2880
    };

    const CCheckpointData &Checkpoints() {
        if (TestNet())
            return dataTestnet;
        else
            return data;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!fEnabled)
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
        if (!fEnabled)
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!fEnabled)
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
