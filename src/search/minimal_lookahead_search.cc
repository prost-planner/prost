#include "minimal_lookahead_search.h"

using namespace std;

MinimalLookaheadSearch::HashMap MinimalLookaheadSearch::rewardCache;

MinimalLookaheadSearch::MinimalLookaheadSearch() :
    DeterministicSearchEngine("MLS"),
    numberOfRuns(0),
    cacheHits(0) {
    if (rewardCache.bucket_count() < 520241) {
        rewardCache.reserve(520241);
    }
}

bool MinimalLookaheadSearch::estimateQValues(State const& current,
                                             vector<int> const& actionsToExpand,
                                             vector<double>& qValues) {
    HashMap::iterator it = rewardCache.find(current);

    if (it != rewardCache.end()) {
        ++cacheHits;
        assert(qValues.size() == it->second.size());
        for (size_t i = 0; i < qValues.size(); ++i) {
            qValues[i] = it->second[i];
        }
    } else {
        if(rewardCPF->isActionIndependent()) {
            // Calculate the reward in the current state. It doesn't matter which
            // action we use for this, since there is no action fluent in the reward
            // formula anyway (it therefore doesn't even matter if the used action is
            // applicable or not)
            double reward = 0.0;
            calcReward(current, 0, reward);

            for(size_t actionIndex = 0; actionIndex < actionsToExpand.size(); ++actionIndex) {
                if(actionsToExpand[actionIndex] == actionIndex) {
                    // Calculate the successor state given this action is applied
                    // (i.e., the action matters here!)
                    State next;
                    calcSuccessorState(current, actionIndex, next);

                    // Now that we have the successor state, we can (again) use any
                    // action to calculate the reward of the next state.
                    double reward2;
                    calcReward(next, 0, reward2);
                    qValues[actionIndex] = (reward + reward2) / 2.0;
                }
            }
        } else if(actionStates[0].scheduledActionFluents.empty() &&
                  actionStates[0].actionPreconditions.empty()) {
            // There is an action fluent in the reward (and the reward in the
            // current state hence depends on the applied action, but it is often
            // the case that only the "bad" part of applying an action is in this
            // result, while the positive effect of the action is not. Since noop is
            // always applicable in this task, we apply in the next state to account
            // for those positive effects.
            for(size_t actionIndex = 0; actionIndex < actionsToExpand.size(); ++actionIndex) {
                if(actionsToExpand[actionIndex] == actionIndex) {
                    State next;
                    double reward;
                    calcStateTransition(current, actionIndex, next, reward);

                    // Use any action to calculate the reward in the next state.
                    double reward2;
                    calcReward(next, 0, reward2);
                    qValues[actionIndex] = (reward + reward2) / 2.0;
                }
            }
        } else {
            // Apply all actions to the current state
            for(size_t actionIndex = 0; actionIndex < actionsToExpand.size(); ++actionIndex) {
                if(actionsToExpand[actionIndex] == actionIndex) {
                    calcReward(current, actionIndex, qValues[actionIndex]);
                }
            }
        }

        if (cachingEnabled) {
            rewardCache[current] = qValues;
        }
        ++numberOfRuns;

    }
    return true;
}

void MinimalLookaheadSearch::printStats(ostream& out,
                                        bool const& /*printRoundStats*/, 
                                        string indent) const {
    out << indent << "Cache hits: " << cacheHits << " (in " << numberOfRuns << " runs)" << endl;
}
