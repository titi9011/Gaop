#include "Controllers/GOAPVehicleController.h"
#include "GOAP/GOAPAction.h"
#include "GOAP/GOAPPlanner.h"
#include "Vehicle/GOAPVehicle.h"
#include "Vehicle/DriveToWaypointAction.h"
#include "Vehicle/WaitAtWaypointAction.h"
#include "Vehicle/WaitForGreenLightAction.h"
#include "Traffic/TrafficLight.h"
#include "Kismet/GameplayStatics.h"

AGOAPVehicleController::AGOAPVehicleController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGOAPVehicleController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicleController] OnPossess: %s"), *GetNameSafe(InPawn));

	// But : on veut que l'agent soit "Resting" (arrivé et ayant attendu)
	GoalState.Set("Resting", true);

	// État initial : la voie est libre par défaut
	CurrentWorldState = FWorldState();
	CurrentWorldState.Set("CanDrive", true);

	// Cache tous les feux de la scène pour éviter GetAllActors chaque frame
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficLight::StaticClass(), Found);
	for (AActor* A : Found)
		if (ATrafficLight* L = Cast<ATrafficLight>(A))
			CachedTrafficLights.Add(L);

	// Choisir une destination initiale aléatoire (seulement si les waypoints sont déjà assignés)
	// Si la route est assignée par RoadGenerator, c'est lui qui appelle PickRandomDestination + Replan.
	if (AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(InPawn))
	{
		if (Vehicle->Waypoints.Num() > 1)
		{
			Vehicle->PickRandomDestination();
			AActor* DestWP = Vehicle->Waypoints.IsValidIndex(Vehicle->DestinationWaypointIndex)
				? Vehicle->Waypoints[Vehicle->DestinationWaypointIndex] : nullptr;
			UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicle] %s → destination waypoint %d (%s)"),
				*GetNameSafe(Vehicle),
				Vehicle->DestinationWaypointIndex,
				*GetNameSafe(DestWP));
		}
	}

	BuildActions();
	Replan();
}

void AGOAPVehicleController::BuildActions()
{
	AvailableActions.Empty();

	// NewObject<> crée les instances dans le contexte de ce controller (GC safe)
	AvailableActions.Add(NewObject<UDriveToWaypointAction>(this));
	AvailableActions.Add(NewObject<UWaitAtWaypointAction>(this));
	AvailableActions.Add(NewObject<UWaitForGreenLightAction>(this));
}

void AGOAPVehicleController::RebuildTrafficLightCache()
{
	CachedTrafficLights.Reset();
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATrafficLight::StaticClass(), Found);
	for (AActor* A : Found)
		if (ATrafficLight* L = Cast<ATrafficLight>(A))
			CachedTrafficLights.Add(L);
}

void AGOAPVehicleController::Replan()
{
	CurrentPlan = FGOAPPlanner::Plan(
		GetPawn(),
		AvailableActions,
		CurrentWorldState,
		GoalState
	);

	CurrentActionIndex = 0;
	bActionActive      = false;

	if (CurrentPlan.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicleController] Aucun plan trouve pour %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[GOAPVehicleController] Plan trouve : %d action(s)"), CurrentPlan.Num());
	}
}

void AGOAPVehicleController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CheckTrafficLightInterrupt();  // Surveille les feux avant d'exécuter l'action
	TickCurrentAction(DeltaTime);
}

void AGOAPVehicleController::TickCurrentAction(float DeltaTime)
{
	static bool bTickLogged = false;
	if (!bTickLogged)
	{
		bTickLogged = true;
		UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicleController] TickCurrentAction — PlanSize=%d ActionIndex=%d bActionActive=%d"),
			CurrentPlan.Num(), CurrentActionIndex, bActionActive ? 1 : 0);
	}

	// Plan épuisé → boucle : remet à zéro et replanifie
	if (!CurrentPlan.IsValidIndex(CurrentActionIndex))
	{
		CurrentWorldState = FWorldState();
		CurrentWorldState.Set("CanDrive", !HasNearbyRedLight());
		Replan();
		return;
	}

	UGOAPAction* Action        = CurrentPlan[CurrentActionIndex];
	APawn*       ControlledPawn = GetPawn();
	if (!Action || !ControlledPawn) return;

	// ── Première frame de cette action : Activate ───────────────────────────
	if (!bActionActive)
	{
		// Vérification de sécurité : les préconditions sont-elles toujours OK ?
		if (!CurrentWorldState.Satisfies(Action->Preconditions))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicleController] Preconditions non satisfaites, on replanifie."));
			Replan();
			return;
		}
		Action->Activate(ControlledPawn);
		bActionActive = true;
	}

	// ── Tick de l'action courante ────────────────────────────────────────────
	Action->Tick(DeltaTime, ControlledPawn);

	// ── L'action est-elle terminée ? ─────────────────────────────────────────
	if (Action->IsFinished(ControlledPawn))
	{
		// Applique les effets sur le WorldState
		CurrentWorldState = CurrentWorldState.Apply(Action->Effects);
		CurrentActionIndex++;
		bActionActive = false;

		UE_LOG(LogTemp, Log, TEXT("[GOAPVehicleController] Action terminée. WorldState mis à jour."));
	}
}

bool AGOAPVehicleController::HasNearbyRedLight() const
{
	APawn* P = GetPawn();
	if (!P) return false;

	const FVector VehicleForward  = P->GetActorForwardVector().GetSafeNormal2D();
	const FVector VehicleLocation = P->GetActorLocation();
	const float   R2              = TrafficLightDetectionRadius * TrafficLightDetectionRadius;

	for (ATrafficLight* L : CachedTrafficLights)
	{
		if (!L || !L->IsBlockingTraffic()) continue;

		const FVector ToLight = L->GetActorLocation() - VehicleLocation;
		if (ToLight.SizeSquared2D() > R2) continue; // trop loin

		// Distance EN AVANT : projection du vecteur vers le feu sur le forward de la voiture.
		// Si le feu est derrière ou trop proche devant, la voiture est déjà engagée → ignorer.
		const float ForwardDist = FVector::DotProduct(ToLight, VehicleForward);
		if (ForwardDist < TrafficLightMinForwardDistance) continue;

		// Le feu doit être devant la voiture (angle)
		const FVector ToLightDir = ToLight.GetSafeNormal2D();
		if (FVector::DotProduct(VehicleForward, ToLightDir) < 0.3f) continue;

		// Le feu doit pointer vers la voiture (seuil strict pour ignorer les feux perpendiculaires)
		const FVector LightForward = L->GetActorForwardVector().GetSafeNormal2D();
		if (FVector::DotProduct(VehicleForward, LightForward) > -0.5f) continue;

		return true;
	}
	return false;
}

void AGOAPVehicleController::CheckTrafficLightInterrupt()
{
	bool bShouldBlock = HasNearbyRedLight();
	bool bCanDriveNow = CurrentWorldState.Get("CanDrive");

	if (bShouldBlock && bCanDriveNow)
	{
		// Un feu rouge vient d'apparaître dans le rayon — interrompre
		CurrentWorldState.Set("CanDrive", false);
		UE_LOG(LogTemp, Warning, TEXT("[GOAPVehicleController] Feu rouge détecté — interruption + replanification."));

		// Interrompre DriveToWaypointAction si c'est l'action active
		if (CurrentPlan.IsValidIndex(CurrentActionIndex) && bActionActive)
		{
			UGOAPAction* Current = CurrentPlan[CurrentActionIndex];
			if (Current && Current->IsA<UDriveToWaypointAction>())
			{
				Current->Abort(GetPawn());
				bActionActive = false;
			}
		}
		Replan();
	}
	else if (!bShouldBlock && !bCanDriveNow)
	{
		// Le feu est passé au vert — WaitForGreenLightAction::Tick le détectera
		// et se terminera seule au prochain IsFinished() == true
		CurrentWorldState.Set("CanDrive", true);
	}
}
