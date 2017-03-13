/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "ModuleFactory.hpp"

using namespace allpix;

// Constructor and destructor
ModuleFactory::ModuleFactory(): conf_(), apx_(nullptr) {}
ModuleFactory::~ModuleFactory() {}

void ModuleFactory::setAllPix(AllPix *allpix) {
    apx_ = allpix;
}

AllPix *ModuleFactory::getAllPix() {
    return apx_;
}

void ModuleFactory::setConfiguration(Configuration conf) {
    conf_ = conf;
}

Configuration &ModuleFactory::getConfiguration() {
    return conf_;
}