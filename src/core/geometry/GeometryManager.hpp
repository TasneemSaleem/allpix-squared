/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 */

#ifndef ALLPIX_GEOMETRY_MANAGER_H
#define ALLPIX_GEOMETRY_MANAGER_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Detector.hpp"

namespace allpix {

    class GeometryManager {
    public:
        // Constructor and destructors
        GeometryManager();
        virtual ~GeometryManager();

        // Disallow copy
        GeometryManager(const GeometryManager&) = delete;
        GeometryManager& operator=(const GeometryManager&) = delete;

        // Get internal representation
        // FIXME: this is not very elegant, but it is probably the only option (possible use a in-between type?)
        void setInternalDescription(std::string, void*);
        void* getInternalDescription(std::string);

        // Add detector to the system
        void addDetector(std::shared_ptr<Detector>);

        // Get detectors
        std::vector<std::shared_ptr<Detector>> getDetectors() const;
        std::shared_ptr<Detector> getDetector(const std::string&) const;
        std::vector<std::shared_ptr<Detector>> getDetectorsByType(const std::string&) const;

    private:
        std::vector<std::shared_ptr<Detector>> detectors_;

        std::set<std::string> detector_names_;
    };
}

#endif /* ALLPIX_GEOMETRY_MANAGER_H */
