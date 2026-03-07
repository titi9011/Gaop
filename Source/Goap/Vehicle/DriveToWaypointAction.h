#pragma once
#include "CoreMinimal.h"
#include "GOAP/GOAPAction.h"
#include "DriveToWaypointAction.generated.h"

/**
 * Action GOAP : conduire jusqu'au waypoint actuel du véhicule.
 *
 * Preconditions : (aucune) — on peut toujours essayer de conduire
 * Effects       : { "AtWaypoint" = true }
 *
 * Algorithme de direction : produit vectoriel entre forward et direction cible
 *   → donne le sens du virage (gauche/droite) sans trigonométrie coûteuse.
 */
UCLASS()
class GOAP_API UDriveToWaypointAction : public UGOAPAction
{
	GENERATED_BODY()

public:
	UDriveToWaypointAction();

	/** Gain proportionnel sur le steering (à ajuster selon la map) */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Drive")
	float SteeringGain = 2.5f;

	/** Accélération de base (0-1) */
	UPROPERTY(EditDefaultsOnly, Category="GOAP|Drive")
	float BaseThrottle = 0.75f;

	virtual void Activate(AActor* Agent) override;
	virtual void Tick(float DeltaTime, AActor* Agent) override;
	virtual bool IsFinished(AActor* Agent) override;
	virtual void Abort(AActor* Agent) override;

private:
	bool bFinished = false;
	bool bLoggedOnce = false;
};
