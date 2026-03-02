#include "Vehicle/DriveToWaypointAction.h"
#include "Vehicle/GOAPVehicle.h"

UDriveToWaypointAction::UDriveToWaypointAction()
{
	Preconditions.Set("CanDrive", true);  // Nécessite que la voie soit libre
	Effects.Set("AtWaypoint", true);
	Cost = 1.f;
}

void UDriveToWaypointAction::Activate(AActor* Agent)
{
	bFinished = false;
}

void UDriveToWaypointAction::Tick(float DeltaTime, AActor* Agent)
{
	AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent);
	if (!Vehicle) { bFinished = true; return; }

	FVector Target = Vehicle->GetCurrentWaypointLocation();
	if (Target.IsZero()) { bFinished = true; return; }

	// ── Arrivée ? ───────────────────────────────────────────────────────────
	float DistSq = FVector::DistSquared2D(Vehicle->GetActorLocation(), Target);
	float Accept = Vehicle->WaypointAcceptRadius;
	if (DistSq <= Accept * Accept)
	{
		Vehicle->SetThrottle(0.f);
		Vehicle->SetSteering(0.f);
		bFinished = true;
		return;
	}

	// ── Direction vers la cible (en 2D, ignore Z) ───────────────────────────
	FVector ToTarget = (Target - Vehicle->GetActorLocation()).GetSafeNormal2D();
	FVector Forward  = Vehicle->GetActorForwardVector().GetSafeNormal2D();

	// CrossProduct Z → positif = cible à droite, négatif = à gauche
	float Cross = FVector::CrossProduct(Forward, ToTarget).Z;
	float Steer = FMath::Clamp(Cross * SteeringGain, -1.f, 1.f);

	// DotProduct → alignement avec la cible (1=droit, 0=perpendiculaire)
	float Dot      = FVector::DotProduct(Forward, ToTarget);
	float Throttle = BaseThrottle * FMath::Max(0.25f, Dot); // ralentir dans les virages

	Vehicle->SetSteering(Steer);
	Vehicle->SetThrottle(Throttle);
}

bool UDriveToWaypointAction::IsFinished(AActor* Agent)
{
	return bFinished;
}

void UDriveToWaypointAction::Abort(AActor* Agent)
{
	if (AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent))
	{
		Vehicle->SetThrottle(0.f);
		Vehicle->SetSteering(0.f);
	}
	bFinished = true;
}
