#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "GOAP/WorldState.h"
#include "GOAPVehicleController.generated.h"

class UGOAPAction;
class ATrafficLight;

/**
 * Controller IA qui pilote un AGOAPVehicle via GOAP.
 *
 * Cycle de vie :
 *   OnPossess  → initialise goal + actions, lance un premier plan
 *   Tick       → exécute l'action courante
 *                → quand l'action finit : applique ses effets, passe à la suivante
 *                → quand le plan est épuisé : remet le WorldState à zéro, replanifie
 *                  (= boucle de patrouille infinie)
 */
UCLASS()
class GOAP_API AGOAPVehicleController : public AAIController
{
	GENERATED_BODY()

public:
	AGOAPVehicleController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	/** Relance la planification (appelable par des systèmes externes comme RoadGenerator) */
	void Replan();

	/** Reconstruit le cache des feux (à appeler après leur spawn par le RoadGenerator) */
	void RebuildTrafficLightCache();

protected:
	/** État du monde désiré (but à atteindre) */
	FWorldState GoalState;

	/** État du monde tel qu'il est perçu par l'agent */
	FWorldState CurrentWorldState;

	/** Toutes les actions disponibles pour ce controller */
	UPROPERTY()
	TArray<UGOAPAction*> AvailableActions;

	/** Plan courant (séquence d'actions à exécuter dans l'ordre) */
	TArray<UGOAPAction*> CurrentPlan;

	/** Index de l'action en cours dans CurrentPlan */
	int32 CurrentActionIndex = 0;

	/** True si l'action courante a déjà reçu Activate() */
	bool bActionActive = false;

	/** Rayon de détection des feux de circulation (cm) */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Traffic")
	float TrafficLightDetectionRadius = 1500.f;

	/** Distance en avant minimale (cm) pour réagir à un feu. Si le feu est plus proche que
	 *  cette valeur dans la direction de conduite, la voiture est déjà engagée dans l'intersection. */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Traffic")
	float TrafficLightMinForwardDistance = 500.f;

private:
	void BuildActions();
	void TickCurrentAction(float DeltaTime);

	/** Cache des feux de la scène (rempli à OnPossess) */
	UPROPERTY()
	TArray<ATrafficLight*> CachedTrafficLights;

	/** Vrai si un feu bloquant est dans le rayon de détection */
	bool HasNearbyRedLight() const;

	/** Surveille les feux chaque frame et interrompt le plan si nécessaire */
	void CheckTrafficLightInterrupt();
};
