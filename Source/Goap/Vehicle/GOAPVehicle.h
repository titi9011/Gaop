#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GOAPVehicle.generated.h"

/**
 * Véhicule GOAP minimaliste — APawn avec physique arcade codée manuellement.
 * Pas de ChaosVehicles : throttle + steering = offset/rotation direct.
 *
 * Usage dans l'éditeur :
 *   1. Créer un Blueprint BP_GOAPVehicle depuis cette classe
 *   2. Assigner un StaticMesh (voiture)
 *   3. Placer des AActor vides comme waypoints dans le niveau
 *   4. Les assigner au tableau Waypoints dans les Details
 */
UCLASS()
class GOAP_API AGOAPVehicle : public APawn
{
	GENERATED_BODY()

public:
	AGOAPVehicle();

	virtual void Tick(float DeltaTime) override;

	// ── Mesh ────────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, Category="Vehicle")
	UStaticMeshComponent* VehicleMesh;

	// ── Paramètres de conduite ───────────────────────────────────────────────
	/** Vitesse maximale en cm/s (800 ≈ 29 km/h) */
	UPROPERTY(EditDefaultsOnly, Category="Vehicle|Drive")
	float MaxSpeed = 800.f;

	/** Vitesse de rotation maximale en degrés/s */
	UPROPERTY(EditDefaultsOnly, Category="Vehicle|Drive")
	float TurnSpeed = 80.f;

	/** Taux d'interpolation pour l'accélération/décélération */
	UPROPERTY(EditDefaultsOnly, Category="Vehicle|Drive")
	float Acceleration = 4.f;

	// ── Waypoints ────────────────────────────────────────────────────────────
	/** Liste ordonnée des points de passage (assigner dans l'éditeur) */
	UPROPERTY(EditInstanceOnly, Category="Vehicle|Patrol")
	TArray<AActor*> Waypoints;

	/** Distance (cm) à partir de laquelle on considère le waypoint atteint */
	UPROPERTY(EditDefaultsOnly, Category="Vehicle|Patrol")
	float WaypointAcceptRadius = 250.f;

	/** Index du waypoint où se trouve actuellement le véhicule */
	UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Navigation")
	int32 CurrentWaypointIndex = 0;

	/** Index du waypoint destination (choisi aléatoirement) */
	UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Navigation")
	int32 DestinationWaypointIndex = 0;

	// ── Interface pour les actions GOAP ─────────────────────────────────────
	/** Applique une entrée d'accélération [-1,1] (négatif = frein/marche arrière) */
	void SetThrottle(float Value);

	/** Applique une entrée de direction [-1,1] (négatif = gauche) */
	void SetSteering(float Value);

	/** Avance d'un pas vers la destination (circulairement) */
	void AdvanceTowardDestination();

	/** Choisit une destination aléatoire parmi les waypoints (≠ courant) */
	void PickRandomDestination();

	/** Vrai si le véhicule est arrivé à sa destination */
	bool HasReachedDestination() const;

	/** Retourne la position du waypoint actuel, ou ZeroVector si aucun */
	FVector GetCurrentWaypointLocation() const;

private:
	float CurrentThrottle = 0.f;
	float CurrentSteering = 0.f;
	float CurrentSpeed    = 0.f;  // vitesse actuelle interpolée
};
