#include "Vehicle/WaitAtWaypointAction.h"
#include "Vehicle/GOAPVehicle.h"

UWaitAtWaypointAction::UWaitAtWaypointAction()
{
	Preconditions.Set("AtWaypoint", true);
	Effects.Set("Resting", true);
	Cost = 1.f;
}

void UWaitAtWaypointAction::Activate(AActor* Agent)
{
	ElapsedTime = 0.f;
	bFinished   = false;

	// Stoppe le véhicule immédiatement
	if (AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent))
	{
		Vehicle->SetThrottle(0.f);
		Vehicle->SetSteering(0.f);
	}
}

void UWaitAtWaypointAction::Tick(float DeltaTime, AActor* Agent)
{
	ElapsedTime += DeltaTime;

	if (ElapsedTime >= WaitDuration)
	{
		// Choisir une nouvelle destination aléatoire avant de signaler la fin
		if (AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent))
			Vehicle->PickRandomDestination();

		bFinished = true;
	}
}

bool UWaitAtWaypointAction::IsFinished(AActor* Agent)
{
	return bFinished;
}
