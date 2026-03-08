#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Traffic/TrafficLight.h"
#include "TrafficLightManager.generated.h"

/**
 * Manager de feux de circulation.
 * Synchronise deux groupes de feux (GroupA / GroupB) en alternance :
 *   Phase 0 : GroupA=Vert,  GroupB=Rouge
 *   Phase 1 : GroupA=Jaune, GroupB=Rouge
 *   Phase 2 : GroupA=Rouge, GroupB=Vert
 *   Phase 3 : GroupA=Rouge, GroupB=Jaune
 *
 * Groupement (généré par RoadGenerator) :
 *   GroupA = SW + NW  (côté gauche de l'intersection)
 *   GroupB = NE + SE  (côté droit)
 *   → feux diagonalement opposés montrent des couleurs contraires.
 *
 * Placer dans le niveau et assigner les feux dans GroupA / GroupB depuis l'éditeur.
 */
UCLASS()
class GOAP_API ATrafficLightManager : public AActor
{
	GENERATED_BODY()

public:
	ATrafficLightManager();
	virtual void BeginPlay() override;

	/** Feux du groupe A (axe principal) */
	UPROPERTY(EditInstanceOnly, Category="Traffic")
	TArray<ATrafficLight*> GroupA;

	/** Feux du groupe B (axe secondaire) */
	UPROPERTY(EditInstanceOnly, Category="Traffic")
	TArray<ATrafficLight*> GroupB;

	/** Durée de la phase verte (secondes) */
	UPROPERTY(EditDefaultsOnly, Category="Traffic")
	float GreenDuration = 5.f;

	/** Durée de la phase jaune (secondes) */
	UPROPERTY(EditDefaultsOnly, Category="Traffic")
	float YellowDuration = 2.f;

private:
	int32        CurrentPhase = 0;  // 0..3
	FTimerHandle TimerHandle;

	void ApplyPhase();
	void AdvancePhase();
	void SetGroupState(TArray<ATrafficLight*>& Group, ETrafficLightState State);
};
