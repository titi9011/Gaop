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
	bLoggedOnce = false;
}

void UDriveToWaypointAction::Tick(float DeltaTime, AActor* Agent)
{
	AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent);
	if (!Vehicle) { bFinished = true; return; }

	FVector Target = Vehicle->GetCurrentWaypointLocation();

	if (!bLoggedOnce)
	{
		bLoggedOnce = true;
		FVector Pos = Vehicle->GetActorLocation();
		UE_LOG(LogTemp, Warning, TEXT("[DriveToWP] Pos=(%.0f,%.0f,%.0f) Target=(%.0f,%.0f,%.0f) IsZero=%d WPIndex=%d WPCount=%d WP[0]Valid=%d"),
			Pos.X, Pos.Y, Pos.Z, Target.X, Target.Y, Target.Z,
			Target.IsZero() ? 1 : 0,
			Vehicle->CurrentWaypointIndex, Vehicle->Waypoints.Num(),
			(Vehicle->Waypoints.Num() > 0 && Vehicle->Waypoints[0] != nullptr) ? 1 : 0);
	}

	if (Target.IsZero()) return; // waypoints pas encore assignés — attendre

	// ── Arrivée au waypoint courant ? ──────────────────────────────────────
	float DistSq = FVector::DistSquared2D(Vehicle->GetActorLocation(), Target);
	float Accept = Vehicle->WaypointAcceptRadius;
	if (DistSq <= Accept * Accept)
	{
		if (Vehicle->HasReachedDestination())
		{
			// Destination finale atteinte — action terminée
			Vehicle->SetThrottle(0.f);
			Vehicle->SetSteering(0.f);
			bFinished = true;
			return;
		}
		// Waypoint intermédiaire — avancer vers la destination
		Vehicle->AdvanceTowardDestination();
		return;
	}

	// ── Direction vers la cible (en 2D, ignore Z) ───────────────────────────
	FVector ToTarget = (Target - Vehicle->GetActorLocation()).GetSafeNormal2D();

	// Conduite à droite : décaler la cible de steering vers la droite du vecteur de marche.
	// Droite du vecteur (dx,dy) en 2D UE = (-dy, dx, 0)
	FVector RightOfTravel  = FVector(-ToTarget.Y, ToTarget.X, 0.f);
	FVector AdjustedTarget = Target + RightOfTravel * LateralOffset;
	FVector ToAdjusted     = (AdjustedTarget - Vehicle->GetActorLocation()).GetSafeNormal2D();

	FVector Forward = Vehicle->GetActorForwardVector().GetSafeNormal2D();

	// CrossProduct Z → positif = cible à droite, négatif = à gauche
	float Cross = FVector::CrossProduct(Forward, ToAdjusted).Z;
	float Steer = FMath::Clamp(Cross * SteeringGain, -1.f, 1.f);

	// DotProduct → alignement avec la cible ajustée (1=droit, 0=perpendiculaire)
	float Dot      = FVector::DotProduct(Forward, ToAdjusted);
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
