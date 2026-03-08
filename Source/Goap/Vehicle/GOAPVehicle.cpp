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

void AGOAPVehicle::AdvanceTowardDestination()
{
	if (Waypoints.Num() > 1)
		CurrentWaypointIndex = (CurrentWaypointIndex + 1) % Waypoints.Num();
}

void AGOAPVehicle::PickRandomDestination()
{
	const int32 N = Waypoints.Num();
	if (N <= 1) return;

	int32 New = FMath::RandRange(0, N - 1);
	// Garantit une destination différente de la position courante
	if (New == CurrentWaypointIndex)
		New = (New + 1) % N;
	DestinationWaypointIndex = New;

	UE_LOG(LogTemp, Log, TEXT("[GOAPVehicle] Nouvelle destination : waypoint %d"), DestinationWaypointIndex);
}

bool AGOAPVehicle::HasReachedDestination() const
{
	return CurrentWaypointIndex == DestinationWaypointIndex;
}

FVector AGOAPVehicle::GetCurrentWaypointLocation() const
{
	if (Waypoints.IsValidIndex(CurrentWaypointIndex) && Waypoints[CurrentWaypointIndex])
		return Waypoints[CurrentWaypointIndex]->GetActorLocation();
	return FVector::ZeroVector;
}
