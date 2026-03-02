#include "GOAP/GOAPPlanner.h"
#include "GOAP/GOAPAction.h"

// Nœud interne du A* forward
struct FPlanNode
{
	FWorldState          State;    // État du monde à ce stade du plan
	TArray<UGOAPAction*> Actions;  // Séquence d'actions pour arriver ici
	float                GCost;   // Coût cumulé depuis le départ
	int32                HCost;   // Heuristique : nombre de faits manquants

	float FCost() const { return GCost + static_cast<float>(HCost); }
};

TArray<UGOAPAction*> FGOAPPlanner::Plan(
	AActor*                        Agent,
	const TArray<UGOAPAction*>&    AvailableActions,
	const FWorldState&             CurrentState,
	const FWorldState&             Goal)
{
	TArray<FPlanNode> Open;
	Open.Add({ CurrentState, {}, 0.f, CurrentState.DistanceTo(Goal) });

	// Limite de sécurité pour éviter les boucles infinies
	constexpr int32 MaxIterations = 500;
	int32 Iterations = 0;

	while (Open.Num() > 0 && Iterations++ < MaxIterations)
	{
		// Nœud avec le plus petit F = G + H
		int32 BestIdx = 0;
		for (int32 i = 1; i < Open.Num(); i++)
			if (Open[i].FCost() < Open[BestIdx].FCost()) BestIdx = i;

		FPlanNode Current = Open[BestIdx];
		Open.RemoveAtSwap(BestIdx);

		// But atteint ?
		if (Current.State.Satisfies(Goal))
			return Current.Actions;

		// Expansion : tester chaque action applicable
		for (UGOAPAction* Action : AvailableActions)
		{
			if (!Current.State.Satisfies(Action->Preconditions)) continue;
			if (!Action->CheckProcedural(Agent))                  continue;

			FWorldState          NextState   = Current.State.Apply(Action->Effects);
			TArray<UGOAPAction*> NextActions = Current.Actions;
			NextActions.Add(Action);

			Open.Add({
				NextState,
				NextActions,
				Current.GCost + Action->Cost,
				NextState.DistanceTo(Goal)
			});
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("GOAPPlanner: Aucun plan trouvé."));
	return {};
}
