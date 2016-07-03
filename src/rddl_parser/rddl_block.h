/**
    rddl_block.h: classes needed for definition and implementation of RDDL program which defined in 2 files (domain and instance file)

    @author Đorđe Relić <dorde.relic@unibas.ch>
    @version 1.0 06/2016
*/

#ifndef RDDL_BLOCK_H
#define RDDL_BLOCK_H

#include <string>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include "planning_task.h"
#include "utils/system_utils.h"
#include "utils/timer.h"
#include "domain.h"
#include "non_fluents.h"
#include "instance.h"
#include "instantiator.h"
#include "preprocessor.h"
#include "task_analyzer.h"

class RDDLBlock {
public:
    RDDLBlock()
        : task(new PlanningTask()) {}
    ~RDDLBlock();
    void addDomain(Domain* d);
    void addInstance(Instance* i);
    void addNonFluent(NonFluentBlock* nf);

    void execute(std::string targetDir);
    PlanningTask *task;

    std::string getDomainName();
    std::string getNonFluentsName();
private:
    std::string domainName;
    std::string nonFluentsName;
};



#endif
