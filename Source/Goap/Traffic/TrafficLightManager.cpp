#include "Traffic/TrafficLightManager.h"

ATrafficLightManager::ATrafficLightManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATrafficLightManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentPhase = FMath::RandRange(0, 3);
	ApplyPhase();
	AdvancePhase();
}

void ATrafficLightManager::ApplyPhase()
{
	switch (CurrentPhase)
	{
	case 0:
		SetGroupState(GroupA, ETrafficLightState::Green);
		SetGroupState(GroupB, ETrafficLightState::Red);
		UE_LOG(LogTemp, Log, TEXT("[TrafficLightManager] Phase 0 : GroupA=Vert, GroupB=Rouge"));
		break;
	case 1:
		SetGroupState(GroupA, ETrafficLightState::Yellow);
		SetGroupState(GroupB, ETrafficLightState::Red);
		UE_LOG(LogTemp, Log, TEXT("[TrafficLightManager] Phase 1 : GroupA=Jaune, GroupB=Rouge"));
		break;
	case 2:
		SetGroupState(GroupA, ETrafficLightState::Red);
		SetGroupState(GroupB, ETrafficLightState::Green);
		UE_LOG(LogTemp, Log, TEXT("[TrafficLightManager] Phase 2 : GroupA=Rouge, GroupB=Vert"));
		break;
	case 3:
		SetGroupState(GroupA, ETrafficLightState::Red);
		SetGroupState(GroupB, ETrafficLightState::Yellow);
		UE_LOG(LogTemp, Log, TEXT("[TrafficLightManager] Phase 3 : GroupA=Rouge, GroupB=Jaune"));
		break;
	default:
		break;
	}
}

void ATrafficLightManager::AdvancePhase()
{
	// Phase paire → durée verte ; phase impaire → durée jaune
	float Delay = (CurrentPhase % 2 == 0) ? GreenDuration : YellowDuration;

	CurrentPhase = (CurrentPhase + 1) % 4;

	GetWorldTimerManager().SetTimer(
		TimerHandle,
		[this]()
		{
			ApplyPhase();
			AdvancePhase();
		},
		Delay,
		false
	);
}

void ATrafficLightManager::SetGroupState(TArray<ATrafficLight*>& Group, ETrafficLightState State)
{
	for (ATrafficLight* Light : Group)
	{
		if (Light)
		{
			Light->SetState(State);
		}
	}
}
