#pragma once
#include "CoreMinimal.h"

/**
 * L'état du monde : une liste de faits booléens nommés.
 * Ex : { "AtWaypoint" = true, "Resting" = false }
 */
struct FWorldState
{
	TMap<FName, bool> Facts;

	void Set(FName Key, bool Value)
	{
		Facts.Add(Key, Value);
	}

	bool Get(FName Key) const
	{
		const bool* V = Facts.Find(Key);
		return V ? *V : false;
	}

	/** Vrai si cet état satisfait tous les faits de Goal */
	bool Satisfies(const FWorldState& Goal) const
	{
		for (const auto& Pair : Goal.Facts)
			if (Get(Pair.Key) != Pair.Value) return false;
		return true;
	}

	/** Nombre de faits de Goal non satisfaits (heuristique A*) */
	int32 DistanceTo(const FWorldState& Goal) const
	{
		int32 D = 0;
		for (const auto& Pair : Goal.Facts)
			if (Get(Pair.Key) != Pair.Value) D++;
		return D;
	}

	/** Retourne un nouvel état avec les effets appliqués par-dessus */
	FWorldState Apply(const FWorldState& Effects) const
	{
		FWorldState Result = *this;
		for (const auto& Pair : Effects.Facts)
			Result.Facts.Add(Pair.Key, Pair.Value);
		return Result;
	}
};
