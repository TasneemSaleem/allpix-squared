/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "VisualizationGeant4Module.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#include <G4LogicalVolume.hh>
#include <G4RunManager.hh>
#ifdef G4UI_USE_QT
#include <G4UIQt.hh>
#endif
#include <G4UImanager.hh>
#include <G4UIsession.hh>
#include <G4UIterminal.hh>
#include <G4VisAttributes.hh>
#include <G4VisExecutive.hh>

#include "core/utils/log.h"

using namespace allpix;

VisualizationGeant4Module::VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager* geo_manager)
    : Module(config), config_(std::move(config)), geo_manager_(geo_manager), has_run_(false), session_param_ptr_(nullptr) {}
VisualizationGeant4Module::~VisualizationGeant4Module() {
    if(!has_run_ && vis_manager_g4_ != nullptr && vis_manager_g4_->GetCurrentViewer() != nullptr &&
       config_.get<std::string>("driver", "") == "VRML2FILE") {
        LOG(TRACE) << "Invoking VRML workaround to prevent visualization under error conditions";

        // FIXME: workaround to skip VRML visualization in case we stopped before reaching the run method
        auto str = getenv("G4VRMLFILE_VIEWER");
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", "NONE", 1);
        }
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", str, 1);
        }
    }
}

void VisualizationGeant4Module::init() {
    // suppress all geant4 output
    SUPPRESS_STREAM(G4cout);

    // check if we have a running G4 manager
    G4RunManager* run_manager_g4 = G4RunManager::GetRunManager();
    if(run_manager_g4 == nullptr) {
        RELEASE_STREAM(G4cout);
        throw ModuleError("Cannot visualize using Geant4 without a Geant4 geometry builder");
    }

    if(config_.has("use_gui")) {
        // Need to provide parameters, simulate this behaviour
        session_param_ = ALLPIX_PROJECT_NAME;
        session_param_ptr_ = const_cast<char*>(session_param_.data()); // NOLINT
#ifdef G4UI_USE_QT
        gui_session_ = std::make_unique<G4UIQt>(1, &session_param_ptr_);
#else
        throw InvalidValueError(
            config_, "use_gui", "GUI session cannot be started because Qt is not available in this Geant4");
#endif
    }

    // Set the visibility attributes for visualization
    set_visibility_attributes();

    // initialize the session and the visualization manager
    LOG(TRACE) << "Initializing visualization";
    vis_manager_g4_ = std::make_unique<G4VisExecutive>("quiet");
    vis_manager_g4_->Initialize();

    // execute standard commands
    // FIXME: should execute this directly and not through the UI
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/vis/scene/create");
    // FIXME: no way to check if this driver actually exists...
    int check_driver = UI->ApplyCommand("/vis/sceneHandler/create " + config_.get<std::string>("driver"));
    if(check_driver != 0) {
        RELEASE_STREAM(G4cout);
        std::set<G4String> candidates;
        for(auto system : vis_manager_g4_->GetAvailableGraphicsSystems()) {
            for(auto& nickname : system->GetNicknames()) {
                if(!nickname.contains("FALLBACK")) {
                    candidates.insert(nickname);
                }
            }
        }

        std::string candidate_str;
        for(auto& candidate : candidates) {
            candidate_str += candidate;
            candidate_str += ", ";
        }
        if(!candidate_str.empty()) {
            candidate_str = candidate_str.substr(0, candidate_str.size() - 2);
        }

        vis_manager_g4_.reset();
        throw InvalidValueError(
            config_, "driver", "visualization driver does not exists (options are " + candidate_str + ")");
    }

    UI->ApplyCommand("/vis/sceneHandler/attach");
    UI->ApplyCommand("/vis/viewer/create");

    // release the stream early in debugging mode
    IFLOG(DEBUG) { RELEASE_STREAM(G4cout); }

    // execute initialization macro if provided
    if(config_.has("macro_init")) {
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_init"));
    }

    // release the g4 output
    RELEASE_STREAM(G4cout);
}

void VisualizationGeant4Module::set_visibility_attributes() {
    // Create all visualization attributes

    // To add some transparency in the solids, set to 0.2. 1 means opaque.
    // Transparency can be switched off in the visualisation.
    const double alpha = config_.get<double>("transparency", 0.2);
    if(alpha < 0 || alpha > 1) {
        throw InvalidValueError(config_, "transparency", "Transparency level should be between 0 and 1");
    }

    // Wrapper
    G4VisAttributes wrapperVisAtt = G4VisAttributes(G4Color(1, 0, 0, 0.1)); // Red
    wrapperVisAtt.SetVisibility(false);

    // PCB
    auto pcbColor = G4Color(0.36, 0.66, 0.055, alpha); // Greenish
    G4VisAttributes pcbVisAtt = G4VisAttributes(pcbColor);
    pcbVisAtt.SetLineWidth(1);
    pcbVisAtt.SetForceSolid(false);

    // Chip
    auto chipColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    G4VisAttributes ChipVisAtt = G4VisAttributes(chipColor);
    ChipVisAtt.SetForceSolid(false);

    // Bumps
    auto bumpColor = G4Color(0.5, 0.5, 0.5, alpha); // Grey
    G4VisAttributes BumpVisAtt = G4VisAttributes(bumpColor);
    BumpVisAtt.SetForceSolid(false);

    // The logical volume holding all the bumps
    G4VisAttributes BumpBoxVisAtt = G4VisAttributes(bumpColor);

    // Sensors, ie pixels
    auto sensorColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    G4VisAttributes SensorVisAtt = G4VisAttributes(chipColor);
    SensorVisAtt.SetForceSolid(false);

    // Guard rings
    G4VisAttributes guardRingsVisAtt = G4VisAttributes(sensorColor);
    guardRingsVisAtt.SetForceSolid(false);

    // The box holding all the pixels
    G4VisAttributes BoxVisAtt = G4VisAttributes(sensorColor);

    // In default simple view mode, pixels and bumps are set to invisible, not to be displayed.
    // The logical volumes holding them are instead displayed.
    auto simple_view = config_.get<bool>("simple_view", true);
    if(simple_view) {
        SensorVisAtt.SetVisibility(false);
        BoxVisAtt.SetVisibility(true);
        BumpVisAtt.SetVisibility(false);
        BumpBoxVisAtt.SetVisibility(true);
    } else {
        SensorVisAtt.SetVisibility(true);
        BoxVisAtt.SetVisibility(false);
        BumpVisAtt.SetVisibility(true);
        BumpBoxVisAtt.SetVisibility(false);
    }

    // Apply the visualization attributes to all detectors that exist
    for(auto& detector : geo_manager_->getDetectors()) {
        auto wrapper_log = detector->getExternalObject<G4LogicalVolume>("wrapper_log");
        if(wrapper_log != nullptr) {
            wrapper_log->SetVisAttributes(wrapperVisAtt);
        }

        auto sensor_log = detector->getExternalObject<G4LogicalVolume>("sensor_log");
        if(sensor_log != nullptr) {
            sensor_log->SetVisAttributes(BoxVisAtt);
        }

        auto slice_log = detector->getExternalObject<G4LogicalVolume>("slice_log");
        if(slice_log != nullptr) {
            slice_log->SetVisAttributes(SensorVisAtt);
        }

        auto pixel_log = detector->getExternalObject<G4LogicalVolume>("pixel_log");
        if(pixel_log != nullptr) {
            pixel_log->SetVisAttributes(SensorVisAtt);
        }

        auto bumps_wrapper_log = detector->getExternalObject<G4LogicalVolume>("bumps_wrapper_log");
        if(bumps_wrapper_log != nullptr) {
            bumps_wrapper_log->SetVisAttributes(BumpBoxVisAtt);
        }

        auto bumps_cell_log = detector->getExternalObject<G4LogicalVolume>("bumps_cell_log");
        if(bumps_cell_log != nullptr) {
            bumps_cell_log->SetVisAttributes(BumpVisAtt);
        }

        auto guard_rings_log = detector->getExternalObject<G4LogicalVolume>("guard_rings_log");
        if(guard_rings_log != nullptr) {
            guard_rings_log->SetVisAttributes(guardRingsVisAtt);
        }

        auto chip_log = detector->getExternalObject<G4LogicalVolume>("chip_log");
        if(chip_log != nullptr) {
            chip_log->SetVisAttributes(ChipVisAtt);
        }

        auto PCB_log = detector->getExternalObject<G4LogicalVolume>("pcb_log");
        if(PCB_log != nullptr) {
            PCB_log->SetVisAttributes(pcbVisAtt);
        }
    }
}

void VisualizationGeant4Module::run(unsigned int) {
    // suppress stream if not in debugging mode
    IFLOG(DEBUG);
    else {
        SUPPRESS_STREAM(G4cout);
    }

    // execute the run macro
    if(config_.has("macro_run")) {
        G4UImanager* UI = G4UImanager::GetUIpointer();
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_run"));
    }

    // release the stream (if it was suspended)
    RELEASE_STREAM(G4cout);
}

// display the visualization after all events have passed
void VisualizationGeant4Module::finalize() {
    // flush the view or open an interactive session depending on settings
    if(config_.has("use_gui")) {
        LOG(INFO) << "Starting visualization session";
        gui_session_->SessionStart();
    } else {
        if(config_.get("interactive", false)) {
            LOG(INFO) << "Starting terminal session";
            std::unique_ptr<G4UIsession> session = std::make_unique<G4UIterminal>();
            session->SessionStart();
        } else {
            LOG(INFO) << "Starting viewer";
            vis_manager_g4_->GetCurrentViewer()->ShowView();
        }
    }

    // set that we did succesfully visualize
    has_run_ = true;
}
