#include "thts.h"

#include "action_selection.h"
#include "backup_function.h"
#include "initializer.h"
#include "outcome_selection.h"
#include "recommendation_function.h"

#include "utils/system_utils.h"

/******************************************************************
                     Search Engine Creation
******************************************************************/

THTS::THTS(std::string _name)
    : ProbabilisticSearchEngine(_name),
      actionSelection(nullptr),
      outcomeSelection(nullptr),
      backupFunction(nullptr),
      initializer(nullptr),
      recommendationFunction(nullptr),
      currentRootNode(nullptr),
      chosenOutcome(nullptr),
      tipNodeOfTrial(nullptr),
      states(SearchEngine::horizon + 1),
      stepsToGoInCurrentState(SearchEngine::horizon),
      stepsToGoInNextState(SearchEngine::horizon - 1),
      appliedActionIndex(-1),
      trialReward(0.0),
      currentTrial(0),
      initializedDecisionNodes(0),
      lastUsedNodePoolIndex(0),
      terminationMethod(THTS::TIME),
      maxNumberOfTrials(0),
      numberOfNewDecisionNodesPerTrial(1),
      numberOfRuns(0),
      cacheHits(0),
      accumulatedNumberOfStepsToGoInFirstSolvedRootState(0),
      firstSolvedFound(false),
      accumulatedNumberOfTrialsInRootState(0),
      accumulatedNumberOfSearchNodesInRootState(0) {
    setMaxNumberOfNodes(24000000);
    setTimeout(1.0);
    setRecommendationFunction(new ExpectedBestArmRecommendation(this));
}

bool THTS::setValueFromString(std::string& param, std::string& value) {
    // Check if this parameter encodes an ingredient
    if (param == "-act") {
        setActionSelection(ActionSelection::fromString(value, this));

        return true;
    } else if (param == "-out") {
        setOutcomeSelection(OutcomeSelection::fromString(value, this));

        return true;
    } else if (param == "-backup") {
        setBackupFunction(BackupFunction::fromString(value, this));

        return true;
    } else if (param == "-init") {
        setInitializer(Initializer::fromString(value, this));
        return true;
    } else if (param == "-rec") {
        setRecommendationFunction(
            RecommendationFunction::fromString(value, this));
        return true;
    }

    if (param == "-T") {
        if (value == "TIME") {
            setTerminationMethod(THTS::TIME);
            return true;
        } else if (value == "TRIALS") {
            setTerminationMethod(THTS::NUMBER_OF_TRIALS);
            return true;
        } else if (value == "TIME_AND_TRIALS") {
            setTerminationMethod(THTS::TIME_AND_NUMBER_OF_TRIALS);
            return true;
        } else {
            return false;
        }
    } else if (param == "-r") {
        setMaxNumberOfTrials(atoi(value.c_str()));
        return true;
    } else if (param == "-ndn") {
        if (value == "H") {
            setNumberOfNewDecisionNodesPerTrial(SearchEngine::horizon);
        } else {
            setNumberOfNewDecisionNodesPerTrial(atoi(value.c_str()));
        }
        return true;
    } else if (param == "-node-limit") {
        setMaxNumberOfNodes(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void THTS::setActionSelection(ActionSelection* _actionSelection) {
    if (actionSelection) {
        delete actionSelection;
    }
    actionSelection = _actionSelection;
}

void THTS::setOutcomeSelection(OutcomeSelection* _outcomeSelection) {
    if (outcomeSelection) {
        delete outcomeSelection;
    }
    outcomeSelection = _outcomeSelection;
}

void THTS::setBackupFunction(BackupFunction* _backupFunction) {
    if (backupFunction) {
        delete backupFunction;
    }
    backupFunction = _backupFunction;
}

void THTS::setInitializer(Initializer* _initializer) {
    if (initializer) {
        delete initializer;
    }
    initializer = _initializer;
}

void THTS::setRecommendationFunction(
    RecommendationFunction* _recommendationFunction) {
    if (recommendationFunction) {
        delete recommendationFunction;
    }
    recommendationFunction = _recommendationFunction;
}

/******************************************************************
                 Search Engine Administration
******************************************************************/

void THTS::disableCaching() {
    actionSelection->disableCaching();
    outcomeSelection->disableCaching();
    backupFunction->disableCaching();
    initializer->disableCaching();
    recommendationFunction->disableCaching();
    SearchEngine::disableCaching();
}

void THTS::learn() {
    // All ingredients must have been specified
    if (!actionSelection || !outcomeSelection || !backupFunction ||
        !initializer || !recommendationFunction) {
        SystemUtils::abort(
            "Action selection, outcome selection, backup "
            "function, initializer, and recommendation function "
            "must be defined in a THTS search engine!");
    }

    std::cout << name << ": learning..." << std::endl;
    actionSelection->learn();
    outcomeSelection->learn();
    backupFunction->learn();
    initializer->learn();
    recommendationFunction->learn();
    std::cout << name << ": ...finished" << std::endl;
}

/******************************************************************
                 Initialization of search phases
******************************************************************/

void THTS::initRound() {
    firstSolvedFound = false;

    actionSelection->initRound();
    outcomeSelection->initRound();
    backupFunction->initRound();
    initializer->initRound();
    recommendationFunction->initRound();
}

void THTS::initStep(State const& _rootState) {
    PDState rootState(_rootState);
    // Adjust maximal search depth and set root state
    if (rootState.stepsToGo() > maxSearchDepth) {
        maxSearchDepthForThisStep = maxSearchDepth;
        states[maxSearchDepthForThisStep].setTo(rootState);
        states[maxSearchDepthForThisStep].stepsToGo() =
            maxSearchDepthForThisStep;
    } else {
        maxSearchDepthForThisStep = rootState.stepsToGo();
        states[maxSearchDepthForThisStep].setTo(rootState);
    }
    assert(states[maxSearchDepthForThisStep].stepsToGo() ==
           maxSearchDepthForThisStep);

    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    // Reset step dependent counter
    currentTrial = 0;
    cacheHits = 0;

    // Reset search nodes and create root node
    currentRootNode = createRootNode();

    std::cout << name << ": Maximal search depth set to "
              << maxSearchDepthForThisStep << std::endl
              << std::endl;
}

inline void THTS::initTrial() {
    // Reset states and steps-to-go counter
    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    // Reset trial dependent variables
    initializedDecisionNodes = 0;
    trialReward = 0.0;
    tipNodeOfTrial = nullptr;

    // Init trial in ingredients
    actionSelection->initTrial();
    outcomeSelection->initTrial();
    backupFunction->initTrial();
    initializer->initTrial();
}

inline void THTS::initTrialStep() {
    --stepsToGoInCurrentState;
    --stepsToGoInNextState;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);
}

/******************************************************************
                       Main Search Functions
******************************************************************/

void THTS::estimateBestActions(State const& _rootState,
                               std::vector<int>& bestActions) {
    assert(bestActions.empty());

    stopwatch.reset();

    // Init round (if this is the first call in a round)
    if (_rootState.stepsToGo() == SearchEngine::horizon) {
        initRound();
    }

    // Init step (this function is currently only called once per step) TODO:
    // maybe we should call initRound, initStep and printStats from "outside"
    // such that we can also use this as a heuristic without generating too much
    // output
    initStep(_rootState);

    // Check if there is an obviously optimal policy (as, e.g., in the last step
    // or in a reward lock)
    int uniquePolicyOpIndex = getUniquePolicy();
    if (uniquePolicyOpIndex != -1) {
        std::cout << "Returning unique policy: ";
        SearchEngine::actionStates[uniquePolicyOpIndex].printCompact(std::cout);
        std::cout << std::endl << std::endl;
        bestActions.push_back(uniquePolicyOpIndex);
        currentRootNode = nullptr;
        printStats(std::cout, (_rootState.stepsToGo() == 1));
        return;
    }

    // Start the main loop that starts trials until some termination criterion
    // is fullfilled
    while (moreTrials()) {
        // std::cout <<
        // "---------------------------------------------------------" <<
        // std::endl;
        // std::cout << "TRIAL " << (currentTrial+1) << std::endl;
        // std::cout <<
        // "---------------------------------------------------------" <<
        // std::endl;
        visitDecisionNode(currentRootNode);
        ++currentTrial;
        // for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        //     if(currentRootNode->children[i]) {
        //         SearchEngine::actionStates[i].print(std::cout);
        //         std::cout << std::endl;
        //         currentRootNode->children[i]->print(std::cout, "  ");
        //     }
        // }
        // assert(currentTrial != 100);
    }

    recommendationFunction->recommend(currentRootNode, bestActions);
    assert(!bestActions.empty());

    // Update statistics
    ++numberOfRuns;

    if (currentRootNode->solved && !firstSolvedFound) {
        // TODO: This is the first root state that was solved, so everything
        // that could happen in the future is also solved. We should (at least
        // in this case) make sure that we keep the tree and simply follow the
        // optimal policy.
        firstSolvedFound = true;
        accumulatedNumberOfStepsToGoInFirstSolvedRootState +=
            _rootState.stepsToGo();
    }

    if (_rootState.stepsToGo() == SearchEngine::horizon) {
        accumulatedNumberOfTrialsInRootState += currentTrial;
        accumulatedNumberOfSearchNodesInRootState += lastUsedNodePoolIndex;
    }

    // Print statistics
    std::cout << "Search time: " << stopwatch << std::endl;
    printStats(std::cout, (_rootState.stepsToGo() == 1));
}

bool THTS::moreTrials() {
    // Check memory constraints and solvedness
    if (currentRootNode->solved ||
        (lastUsedNodePoolIndex >= maxNumberOfNodes)) {
        return false;
    }

    if (currentTrial == 0) {
        return true;
    }

    // Check selected termination criterion
    switch (terminationMethod) {
    case THTS::TIME:
        if (MathUtils::doubleIsGreater(stopwatch(), timeout)) {
            return false;
        }
        break;
    case THTS::NUMBER_OF_TRIALS:
        if (currentTrial == maxNumberOfTrials) {
            return false;
        }
        break;
    case THTS::TIME_AND_NUMBER_OF_TRIALS:
        if (MathUtils::doubleIsGreater(stopwatch(), timeout) ||
            (currentTrial == maxNumberOfTrials)) {
            return false;
        }
        break;
    }

    return true;
}

void THTS::visitDecisionNode(SearchNode* node) {
    if (node == currentRootNode) {
        initTrial();
    } else {
        // Continue trial (i.e., set next state to be the current)
        initTrialStep();

        // Check if there is a "special" reason to stop this trial (currently,
        // this is the case if the state value of the current state is cached,
        // if it is a reward lock or if there is only one step left).
        if (currentStateIsSolved(node)) {
            if (!tipNodeOfTrial) {
                tipNodeOfTrial = node;
            }
            return;
        }
    }

    // Initialize node if necessary
    if (!node->initialized) {
        if (!tipNodeOfTrial) {
            tipNodeOfTrial = node;
        }

        initializer->initialize(node, states[stepsToGoInCurrentState]);

        if (node != currentRootNode) {
            ++initializedDecisionNodes;
        }
    }

    // std::cout << std::endl << std::endl << "Current state is: " << std::endl;
    // states[stepsToGoInCurrentState].printCompact(std::cout);
    // std::cout << "Reward is " << node->immediateReward << std::endl;

    // Determine if we continue with this trial
    if (continueTrial(node)) {
        // Select the action that is simulated
        appliedActionIndex = actionSelection->selectAction(node);
        assert(node->children[appliedActionIndex]);
        assert(!node->children[appliedActionIndex]->solved);

        // std::cout << "Chosen action is: ";
        // SearchEngine::actionStates[appliedActionIndex].printCompact(std::cout);
        // std::cout << std::endl;

        // Sample successor state
        calcSuccessorState(states[stepsToGoInCurrentState], appliedActionIndex,
                           states[stepsToGoInNextState]);

        // std::cout << "Sampled PDState is " << std::endl;
        // states[stepsToGoInNextState].printPDStateCompact(std::cout);
        // std::cout << std::endl;

        lastProbabilisticVarIndex = -1;
        for (unsigned int i = 0; i < State::numberOfProbabilisticStateFluents;
             ++i) {
            if (states[stepsToGoInNextState]
                    .probabilisticStateFluentAsPD(i)
                    .isDeterministic()) {
                states[stepsToGoInNextState].probabilisticStateFluent(i) =
                    states[stepsToGoInNextState]
                        .probabilisticStateFluentAsPD(i)
                        .values[0];
            } else {
                lastProbabilisticVarIndex = i;
            }
        }

        // Start outcome selection with the first probabilistic variable
        chanceNodeVarIndex = 0;

        // Continue trial with chance nodes
        if (lastProbabilisticVarIndex < 0) {
            visitDummyChanceNode(node->children[appliedActionIndex]);
        } else {
            visitChanceNode(node->children[appliedActionIndex]);
        }

        // Backup this node
        backupFunction->backupDecisionNode(node);
        trialReward += node->immediateReward;

        // If the backup function labeled the node as solved, we store the
        // result for the associated state in case we encounter it somewhere
        // else in the tree in the future
        if (node->solved) {
            if (cachingEnabled &&
                ProbabilisticSearchEngine::stateValueCache.find(
                    states[node->stepsToGo]) ==
                    ProbabilisticSearchEngine::stateValueCache.end()) {
                ProbabilisticSearchEngine::stateValueCache
                    [states[node->stepsToGo]] =
                        node->getExpectedFutureRewardEstimate();
            }
        }
    } else {
        // The trial is finished
        trialReward = node->getExpectedRewardEstimate();
    }
}

bool THTS::currentStateIsSolved(SearchNode* node) {
    if (stepsToGoInCurrentState == 1) {
        // This node is a leaf (there is still a last decision, though, but that
        // is taken care of by calcOptimalFinalReward)

        calcOptimalFinalReward(states[1], trialReward);
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        return true;
    } else if (ProbabilisticSearchEngine::stateValueCache.find(
                   states[stepsToGoInCurrentState]) !=
               ProbabilisticSearchEngine::stateValueCache.end()) {
        // This state has already been solved before
        trialReward = ProbabilisticSearchEngine::stateValueCache
            [states[stepsToGoInCurrentState]];
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        ++cacheHits;
        return true;
    } else if (node->children.empty() &&
               isARewardLock(states[stepsToGoInCurrentState])) {
        // This state is a reward lock, i.e. a goal or a state that is such that
        // no matter which action is applied we'll always get the same reward

        calcReward(states[stepsToGoInCurrentState], 0, trialReward);
        trialReward *= stepsToGoInCurrentState;
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        if (cachingEnabled) {
            assert(ProbabilisticSearchEngine::stateValueCache.find(
                       states[stepsToGoInCurrentState]) ==
                   ProbabilisticSearchEngine::stateValueCache.end());
            ProbabilisticSearchEngine::stateValueCache
                [states[stepsToGoInCurrentState]] =
                    node->getExpectedFutureRewardEstimate();
        }
        return true;
    }
    return false;
}

void THTS::visitChanceNode(SearchNode* node) {
    while (states[stepsToGoInNextState]
               .probabilisticStateFluentAsPD(chanceNodeVarIndex)
               .isDeterministic()) {
        ++chanceNodeVarIndex;
    }

    chosenOutcome = outcomeSelection->selectOutcome(
        node, states[stepsToGoInNextState], chanceNodeVarIndex,
        lastProbabilisticVarIndex);

    if (chanceNodeVarIndex == lastProbabilisticVarIndex) {
        State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
        State::calcStateHashKey(states[stepsToGoInNextState]);

        visitDecisionNode(chosenOutcome);
    } else {
        ++chanceNodeVarIndex;
        visitChanceNode(chosenOutcome);
    }
    backupFunction->backupChanceNode(node, trialReward);
}

void THTS::visitDummyChanceNode(SearchNode* node) {
    State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
    State::calcStateHashKey(states[stepsToGoInNextState]);

    if (node->children.empty()) {
        node->children.resize(1, nullptr);
        node->children[0] = createDecisionNode(1.0);
    }
    assert(node->children.size() == 1);

    visitDecisionNode(node->children[0]);
    backupFunction->backupChanceNode(node, trialReward);
}

/******************************************************************
                      Root State Analysis
******************************************************************/

int THTS::getUniquePolicy() {
    if (stepsToGoInCurrentState == 1) {
        std::cout << "Returning the optimal last action!" << std::endl;
        return getOptimalFinalActionIndex(states[1]);
    }

    std::vector<int> actionsToExpand =
        getApplicableActions(states[stepsToGoInCurrentState]);

    if (isARewardLock(states[stepsToGoInCurrentState])) {
        std::cout << "Current root state is a reward lock state!" << std::endl;
        states[stepsToGoInCurrentState].print(std::cout);
        for (unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if (actionsToExpand[i] == i) {
                return i;
            }
        }

        assert(false);
    }

    std::vector<int> applicableActionIndices =
        getIndicesOfApplicableActions(states[stepsToGoInCurrentState]);
    assert(!applicableActionIndices.empty());

    if (applicableActionIndices.size() == 1) {
        std::cout << "Only one reasonable action in current root state!"
                  << std::endl;
        return applicableActionIndices[0];
    }

    // There is more than one applicable action
    return -1;
}

/******************************************************************
                        Memory management
******************************************************************/

SearchNode* THTS::createRootNode() {
    for (SearchNode* node : nodePool) {
        if (node) {
            if (!node->children.empty()) {
                std::vector<SearchNode*> tmp;
                node->children.swap(tmp);
            }
        } else {
            break;
        }
    }

    SearchNode* res = nodePool[0];

    if (res) {
        res->reset(1.0, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(1.0, stepsToGoInCurrentState);
        nodePool[0] = res;
    }
    res->immediateReward = 0.0;

    lastUsedNodePoolIndex = 1;
    return res;
}

SearchNode* THTS::createDecisionNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInNextState);
    } else {
        res = new SearchNode(prob, stepsToGoInNextState);
        nodePool[lastUsedNodePoolIndex] = res;
    }
    calcReward(states[stepsToGoInCurrentState], appliedActionIndex,
               res->immediateReward);

    ++lastUsedNodePoolIndex;
    return res;
}

SearchNode* THTS::createChanceNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(prob, stepsToGoInCurrentState);
        nodePool[lastUsedNodePoolIndex] = res;
    }

    ++lastUsedNodePoolIndex;
    return res;
}

/******************************************************************
                       Parameter Setter
******************************************************************/

void THTS::setMaxSearchDepth(int _maxSearchDepth) {
    SearchEngine::setMaxSearchDepth(_maxSearchDepth);

    if (initializer) {
        initializer->setMaxSearchDepth(_maxSearchDepth);
    }
}

/******************************************************************
                            Print
******************************************************************/

void THTS::printStats(std::ostream& out, bool const& printRoundStats,
                      std::string indent) const {
    SearchEngine::printStats(out, printRoundStats, indent);

    if (currentTrial > 0) {
        out << indent << "Performed trials: " << currentTrial << std::endl;
        out << indent << "Created SearchNodes: " << lastUsedNodePoolIndex
            << std::endl;
        out << indent << "Cache Hits: " << cacheHits << std::endl;
        actionSelection->printStats(out, indent);
        outcomeSelection->printStats(out, indent);
        backupFunction->printStats(out, indent);
    }
    if (initializer) {
        initializer->printStats(out, printRoundStats, indent + "  ");
    }

    if (currentRootNode) {
        out << std::endl << indent << "Root Node: " << std::endl;
        currentRootNode->print(out);
        out << std::endl << "Q-Value Estimates: " << std::endl;
        for (unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
            if (currentRootNode->children[i]) {
                out << indent;
                SearchEngine::actionStates[i].printCompact(out);
                out << ": ";
                currentRootNode->children[i]->print(out);
            }
        }
    }

    if (printRoundStats) {
        out << std::endl << indent << "ROUND FINISHED" << std::endl;
        out << indent << "Accumulated number of remaining steps in first "
                         "solved root state: "
            << accumulatedNumberOfStepsToGoInFirstSolvedRootState << std::endl;
        out << indent << "Accumulated number of trials in root state: "
            << accumulatedNumberOfTrialsInRootState << std::endl;
        out << indent << "Accumulated number of search nodes in root state: "
            << accumulatedNumberOfSearchNodesInRootState << std::endl;
    }
}
