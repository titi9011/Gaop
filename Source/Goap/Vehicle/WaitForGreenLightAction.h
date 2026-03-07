#pragma once
#include "CoreMinimal.h"
#include "GOAP/GOAPAction.h"
#include "WaitForGreenLightAction.generated.h"

class ATrafficLight;

/**
 * Action GOAP : attendre qu'un feu de circulation proche passe au vert.
 *
 * Preconditions  : {} (aucune)
 * Effects        : { "CanDrive" = true }
 * Cost           : 5.f
 *
 * CheckProcedural retourne true uniquement si un feu bloquant est détecté
 * dans DetectionRadius — le planificateur A* n'inclut donc cette action
 * que lorsqu'un feu rouge est effectivement proche.
 */
UCLASS()
class GOAP_API UWaitForGreenLightAction : public UGOAPAction
{
	GENERATED_BODY()

public:
	UWaitForGreenLightAction();

	/** Rayon de détection des feux de circulation (cm) */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Traffic")
	float DetectionRadius = 1500.f;

	/** Distance en avant minimale (cm) — en-dessous, la voiture est déjà engagée */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Traffic")
	float MinForwardDistance = 500.f;

	virtual bool CheckProcedural(AActor* Agent) override;
	virtual void Activate(AActor* Agent) override;
	virtual void Tick(float DeltaTime, AActor* Agent) override;
	virtual bool IsFinished(AActor* Agent) override;
	virtual void Abort(AActor* Agent) override;

private:
	UPROPERTY()
	ATrafficLight* MonitoredLight = nullptr;

	bool bFinished = false;

	/** Trouve le feu le plus proche qui bloque la circulation dans DetectionRadius */
	ATrafficLight* FindNearestBlockingLight(AActor* Agent) const;
};
