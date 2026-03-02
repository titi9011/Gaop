#include "Vehicle/GOAPVehicle.h"
#include "Controllers/GOAPVehicleController.h"
#include "Components/StaticMeshComponent.h"

AGOAPVehicle::AGOAPVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
	SetRootComponent(VehicleMesh);

	// Le GOAPVehicleController prend automatiquement possession du pawn
	AIControllerClass      = AGOAPVehicleController::StaticClass();
	AutoPossessAI          = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AGOAPVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── Rotation (steering) ─────────────────────────────────────────────────
	// On applique la rotation seulement si le véhicule avance
	if (FMath::Abs(CurrentSpeed) > 10.f)
	{
		float YawDelta = CurrentSteering * TurnSpeed * DeltaTime;
		AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));
	}

	// ── Translation (throttle) ──────────────────────────────────────────────
	float TargetSpeed  = CurrentThrottle * MaxSpeed;
	CurrentSpeed       = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, Acceleration);

	FVector Delta = GetActorForwardVector() * CurrentSpeed * DeltaTime;
	AddActorWorldOffset(Delta, true); // sweep=true → collisions détectées
}

// ── Interface actions GOAP ──────────────────────────────────────────────────

void AGOAPVehicle::SetThrottle(float Value)
{
	CurrentThrottle = FMath::Clamp(Value, -1.f, 1.f);
}

void AGOAPVehicle::SetSteering(float Value)
{
	CurrentSteering = FMath::Clamp(Value, -1.f, 1.f);
}

void AGOAPVehicle::AdvanceWaypoint()
{
	if (Waypoints.Num() > 0)
		CurrentWaypointIndex = (CurrentWaypointIndex + 1) % Waypoints.Num();
}

FVector AGOAPVehicle::GetCurrentWaypointLocation() const
{
	if (Waypoints.IsValidIndex(CurrentWaypointIndex) && Waypoints[CurrentWaypointIndex])
		return Waypoints[CurrentWaypointIndex]->GetActorLocation();
	return FVector::ZeroVector;
}
