#include "SetUniqueTrackIDUserHookG4.hpp"
#include "TrackInfoG4.hpp"

using namespace allpix;

void SetUniqueTrackIDUserHookG4::PreUserTrackingAction(const G4Track* aTrack) {
    auto theTrack = const_cast<G4Track*>(aTrack); // NOLINT
    if(aTrack->GetUserInformation() == nullptr) {
        theTrack->SetUserInformation(new TrackInfoG4(aTrack));
    }
}