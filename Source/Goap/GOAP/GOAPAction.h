#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GOAP/WorldState.h"
#include "GOAPAction.generated.h"

/**
 * Classe de base abstraite pour toutes les actions GOAP.
 *
 * Pour créer une action concrète :
 *   1. Hérite de UGOAPAction
 *   2. Dans le constructeur : remplis Preconditions et Effects
 *   3. Override Activate / Tick / IsFinished
 */
UCLASS(Abstract)
class GOAP_API UGOAPAction : public UObject
{
	GENERATED_BODY()

public:
	/** Conditions du monde requises pour exécuter cette action */
	FWorldState Preconditions;

	/** Ce que cette action change dans le monde une fois terminée */
	FWorldState Effects;

	/** Coût de planification (plus bas = préféré par le planificateur) */
	UPROPERTY(EditDefaultsOnly, Category="GOAP")
	float Cost = 1.0f;

	/** Vérifications procédurales supplémentaires (ex: "est-ce qu'il reste du carburant ?") */
	virtual bool CheckProcedural(AActor* Agent) { return true; }

	/** Appelé une fois quand l'action démarre */
	virtual void Activate(AActor* Agent) {}

	/** Appelé chaque frame pendant l'exécution */
	virtual void Tick(float DeltaTime, AActor* Agent) {}

	/** Retourne true quand l'action a terminé son travail */
	virtual bool IsFinished(AActor* Agent) { return true; }

	/** Appelé si l'action est interrompue */
	virtual void Abort(AActor* Agent) {}
};
