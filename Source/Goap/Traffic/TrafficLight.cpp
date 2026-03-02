#include "Traffic/TrafficLight.h"

ATrafficLight::ATrafficLight()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	LightRed    = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightRed"));
	LightYellow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightYellow"));
	LightGreen  = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightGreen"));

	LightRed->SetupAttachment(Root);
	LightYellow->SetupAttachment(Root);
	LightGreen->SetupAttachment(Root);
}

void ATrafficLight::BeginPlay()
{
	Super::BeginPlay();
	RefreshVisibility();
}

void ATrafficLight::SetState(ETrafficLightState NewState)
{
	CurrentState = NewState;
	RefreshVisibility();
}

bool ATrafficLight::IsBlockingTraffic() const
{
	return CurrentState == ETrafficLightState::Red
		|| CurrentState == ETrafficLightState::Yellow;
}

void ATrafficLight::RefreshVisibility()
{
	if (LightRed)    LightRed->SetVisibility(CurrentState == ETrafficLightState::Red);
	if (LightYellow) LightYellow->SetVisibility(CurrentState == ETrafficLightState::Yellow);
	if (LightGreen)  LightGreen->SetVisibility(CurrentState == ETrafficLightState::Green);
}
