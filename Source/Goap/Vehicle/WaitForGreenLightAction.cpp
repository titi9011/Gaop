#include "Vehicle/WaitForGreenLightAction.h"
#include "Traffic/TrafficLight.h"
#include "Vehicle/GOAPVehicle.h"
#include "Kismet/GameplayStatics.h"

UWaitForGreenLightAction::UWaitForGreenLightAction()
{
	// Pas de préconditions : cette action est planifiable depuis n'importe quel état
	Effects.Set("CanDrive", true);
	Cost = 5.f;
}

bool UWaitForGreenLightAction::CheckProcedural(AActor* Agent)
{
	// Le planificateur A* n'inclut cette action que si un feu rouge est proche
	return FindNearestBlockingLight(Agent) != nullptr;
}

void UWaitForGreenLightAction::Activate(AActor* Agent)
{
	bFinished      = false;
	MonitoredLight = FindNearestBlockingLight(Agent);

	if (!MonitoredLight)
	{
		// Le feu a disparu entre la planification et l'exécution
		bFinished = true;
		return;
	}

	// Immobiliser le véhicule
	if (AGOAPVehicle* Vehicle = Cast<AGOAPVehicle>(Agent))
	{
		Vehicle->SetThrottle(0.f);
		Vehicle->SetSteering(0.f);
	}

	UE_LOG(LogTemp, Log, TEXT("[WaitForGreenLightAction] Feu rouge détecté — véhicule immobilisé."));
}

void UWaitForGreenLightAction::Tick(float DeltaTime, AActor* Agent)
{
	// Terminé si le feu n'existe plus ou n'est plus bloquant
	if (!MonitoredLight || !MonitoredLight->IsBlockingTraffic())
	{
		UE_LOG(LogTemp, Log, TEXT("[WaitForGreenLightAction] Feu vert — reprise de la conduite."));
		bFinished = true;
	}
}

bool UWaitForGreenLightAction::IsFinished(AActor* Agent)
{
	return bFinished;
}

void UWaitForGreenLightAction::Abort(AActor* Agent)
{
	MonitoredLight = nullptr;
	bFinished      = true;
}

ATrafficLight* UWaitForGreenLightAction::FindNearestBlockingLight(AActor* Agent) const
{
	if (!Agent) return nullptr;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		Agent->GetWorld(),
		ATrafficLight::StaticClass(),
		Found
	);

	ATrafficLight* Nearest       = nullptr;
	float          NearestDistSq = DetectionRadius * DetectionRadius;

	const FVector VehicleForward  = Agent->GetActorForwardVector().GetSafeNormal2D();
	const FVector VehicleLocation = Agent->GetActorLocation();

	for (AActor* A : Found)
	{
		ATrafficLight* Light = Cast<ATrafficLight>(A);
		if (!Light || !Light->IsBlockingTraffic()) continue;

		const FVector ToLight = Light->GetActorLocation() - VehicleLocation;

		// Distance en avant : si le feu est derrière ou trop proche, la voiture
		// est déjà engagée dans l'intersection → ignorer
		const float ForwardDist = FVector::DotProduct(ToLight, VehicleForward);
		if (ForwardDist < MinForwardDistance) continue;

		// Le feu doit être devant la voiture (angle)
		const FVector ToLightDir = ToLight.GetSafeNormal2D();
		if (FVector::DotProduct(VehicleForward, ToLightDir) < 0.3f) continue;

		// Le feu doit pointer vers la voiture (directions opposées → dot < -0.5)
		const FVector LightForward = Light->GetActorForwardVector().GetSafeNormal2D();
		if (FVector::DotProduct(VehicleForward, LightForward) > -0.5f) continue;

		float DistSq = ToLight.SizeSquared2D();
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest       = Light;
		}
	}

	return Nearest;
}
