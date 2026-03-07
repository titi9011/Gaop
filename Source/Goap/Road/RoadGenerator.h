#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Traffic/TrafficLight.h"
#include "Traffic/TrafficLightManager.h"
#include "RoadGenerator.generated.h"

/** Nœud de la grille routière */
USTRUCT()
struct FRoadNode
{
	GENERATED_BODY()

	FVector      Location;
	TArray<int32> Neighbors;  // indices des nœuds voisins (droite / haut)
};

/**
 * Génère procéduralement au BeginPlay :
 *   - Un réseau de routes en grille N×M (mesh procédural unifié)
 *   - Des AActor waypoints à chaque nœud
 *   - Des feux tricolores aux intersections 4 voies intérieures
 *   - L'assignation automatique de routes aux AGOAPVehicle du niveau
 *
 * Usage : placer un ARoadGenerator dans le niveau, assigner les classes de feux.
 */
UCLASS()
class GOAP_API ARoadGenerator : public AActor
{
	GENERATED_BODY()

public:
	ARoadGenerator();

	virtual void BeginPlay() override;

	// ── Paramètres de grille ────────────────────────────────────────────────
	/** Nombre de blocs horizontaux */
	UPROPERTY(EditAnywhere, Category="Road|Grid")
	int32 GridWidth = 3;

	/** Nombre de blocs verticaux */
	UPROPERTY(EditAnywhere, Category="Road|Grid")
	int32 GridHeight = 3;

	/** Espacement entre nœuds (cm) */
	UPROPERTY(EditAnywhere, Category="Road|Grid")
	float BlockSize = 10000.f;

	/** Largeur de route (cm) */
	UPROPERTY(EditAnywhere, Category="Road|Grid")
	float RoadWidth = 1000.f;

	// ── Feux de circulation ─────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Road|TrafficLights")
	bool bSpawnTrafficLights = true;

	UPROPERTY(EditAnywhere, Category="Road|TrafficLights")
	TSubclassOf<ATrafficLight> TrafficLightClass;

	UPROPERTY(EditAnywhere, Category="Road|TrafficLights")
	TSubclassOf<ATrafficLightManager> TrafficLightManagerClass;

	// ── Véhicules ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Road|Vehicles")
	bool bAutoAssignRoutes = true;

	// ── Matériau ────────────────────────────────────────────────────────────
	/** Matériau optionnel appliqué au mesh de route (gris par défaut) */
	UPROPERTY(EditAnywhere, Category="Road|Visual")
	UMaterialInterface* RoadMaterial = nullptr;

	// ── Accès aux routes ────────────────────────────────────────────────────
	/** Route périmétrique clockwise (coins + bords) */
	UFUNCTION(BlueprintCallable, Category="Road")
	TArray<AActor*> GetPerimeterRoute() const;

	/** Route horizontale pour la ligne j (0..GridHeight) */
	UFUNCTION(BlueprintCallable, Category="Road")
	TArray<AActor*> GetRowRoute(int32 j) const;

	/** Route verticale pour la colonne i (0..GridWidth) */
	UFUNCTION(BlueprintCallable, Category="Road")
	TArray<AActor*> GetColumnRoute(int32 i) const;

	/** Ré-assigne manuellement les routes aux véhicules */
	UFUNCTION(BlueprintCallable, Category="Road")
	void ForceAssignRoutes();

private:
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* RoadMesh;

	// Données internes
	TArray<FRoadNode> Nodes;
	TArray<AActor*>   SpawnedWaypoints;

	// Index dans Nodes : colonne i, ligne j
	int32 NodeIndex(int32 i, int32 j) const { return i + j * (GridWidth + 1); }

	// Étapes de génération
	void GenerateGrid();
	void BuildRoadMesh();
	void SpawnWaypoints();
	void SpawnTrafficLights();
	void AssignVehicleRoutes();

	// Utilitaire : waypoint le plus proche du véhicule dans une route
	int32 FindNearestWaypointIndex(class AGOAPVehicle* Vehicle, const TArray<AActor*>& Route) const;

	// Helpers mesh
	void AddRoadSegment(TArray<FVector>& Verts, TArray<int32>& Tris,
	                    const FVector& A, const FVector& B);
	void AddIntersectionSquare(TArray<FVector>& Verts, TArray<int32>& Tris,
	                           const FVector& Center);
};
