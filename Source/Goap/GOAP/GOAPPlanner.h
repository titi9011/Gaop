#pragma once
#include "CoreMinimal.h"
#include "GOAP/WorldState.h"

class UGOAPAction;

/**
 * Planificateur GOAP stateless.
 * Utilise un A* "forward" : part de l'état actuel et cherche le chemin vers le Goal.
 */
class GOAP_API FGOAPPlanner
{
public:
	/**
	 * Retourne la séquence d'actions la moins coûteuse pour atteindre Goal.
	 * Retourne un tableau vide si aucun plan n'est trouvé.
	 *
	 * @param Agent           L'acteur qui exécute le plan (pour CheckProcedural)
	 * @param AvailableActions Toutes les actions disponibles
	 * @param CurrentState    L'état du monde en ce moment
	 * @param Goal            L'état désiré
	 */
	static TArray<UGOAPAction*> Plan(
		AActor* Agent,
		const TArray<UGOAPAction*>& AvailableActions,
		const FWorldState& CurrentState,
		const FWorldState& Goal);
};
