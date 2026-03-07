#pragma once
#include "CoreMinimal.h"
#include "GOAP/GOAPAction.h"
#include "WaitAtWaypointAction.generated.h"

/**
 * Action GOAP : s'arrêter au waypoint, attendre, puis avancer au suivant.
 *
 * Preconditions : { "AtWaypoint" = true }
 * Effects       : { "Resting"    = true }
 *
 * Quand l'attente est terminée, on appelle Vehicle->AdvanceWaypoint()
 * pour que le prochain plan conduise vers le waypoint suivant.
 */
UCLASS()
class GOAP_API UWaitAtWaypointAction : public UGOAPAction
{
	GENERATED_BODY()

public:
	UWaitAtWaypointAction();

	/** Durée d'attente en secondes */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Wait")
	float WaitDuration = 0.f;

	virtual void Activate(AActor* Agent) override;
	virtual void Tick(float DeltaTime, AActor* Agent) override;
	virtual bool IsFinished(AActor* Agent) override;

private:
	float ElapsedTime = 0.f;
	bool  bFinished   = false;
};
