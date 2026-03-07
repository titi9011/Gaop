#include "Road/RoadGenerator.h"
#include "Vehicle/GOAPVehicle.h"
#include "EngineUtils.h"           // TActorIterator
#include "Engine/World.h"

ARoadGenerator::ARoadGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	RoadMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RoadMesh"));
	RootComponent = RoadMesh;
	RoadMesh->bUseAsyncCooking = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// BeginPlay — point d'entrée de toute la génération
// ─────────────────────────────────────────────────────────────────────────────

void ARoadGenerator::BeginPlay()
{
	Super::BeginPlay();

	GenerateGrid();
	BuildRoadMesh();
	SpawnWaypoints();

	if (bSpawnTrafficLights)
		SpawnTrafficLights();

	if (bAutoAssignRoutes)
		AssignVehicleRoutes();

	UE_LOG(LogTemp, Log,
		TEXT("[RoadGenerator] Nœuds: %d, Segments: %d"),
		Nodes.Num(),
		(GridWidth * (GridHeight + 1)) + (GridHeight * (GridWidth + 1)));
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. GenerateGrid
// ─────────────────────────────────────────────────────────────────────────────

void ARoadGenerator::GenerateGrid()
{
	Nodes.Reset();
	const int32 Cols = GridWidth  + 1;
	const int32 Rows = GridHeight + 1;
	Nodes.SetNum(Cols * Rows);

	const FVector Origin = GetActorLocation();

	for (int32 j = 0; j < Rows; ++j)
	{
		for (int32 i = 0; i < Cols; ++i)
		{
			FRoadNode& Node = Nodes[NodeIndex(i, j)];
			Node.Location   = Origin + FVector(i * BlockSize, j * BlockSize, 5.f);

			// Voisin à droite
			if (i + 1 < Cols)
			{
				Node.Neighbors.Add(NodeIndex(i + 1, j));
				Nodes[NodeIndex(i + 1, j)].Neighbors.Add(NodeIndex(i, j));
			}
			// Voisin en haut (Y+)
			if (j + 1 < Rows)
			{
				Node.Neighbors.Add(NodeIndex(i, j + 1));
				Nodes[NodeIndex(i, j + 1)].Neighbors.Add(NodeIndex(i, j));
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. BuildRoadMesh
// ─────────────────────────────────────────────────────────────────────────────

void ARoadGenerator::AddRoadSegment(TArray<FVector>& Verts, TArray<int32>& Tris,
                                    const FVector& A, const FVector& B)
{
	const FVector Dir   = (B - A).GetSafeNormal();
	const FVector Up    = FVector::UpVector;
	const FVector Right = FVector::CrossProduct(Dir, Up) * (RoadWidth * 0.5f);

	const int32 Base = Verts.Num();
	Verts.Add(A - Right);  // 0
	Verts.Add(A + Right);  // 1
	Verts.Add(B + Right);  // 2
	Verts.Add(B - Right);  // 3

	Tris.Add(Base + 0); Tris.Add(Base + 2); Tris.Add(Base + 1);
	Tris.Add(Base + 0); Tris.Add(Base + 3); Tris.Add(Base + 2);
}

void ARoadGenerator::AddIntersectionSquare(TArray<FVector>& Verts, TArray<int32>& Tris,
                                            const FVector& Center)
{
	const float HW = RoadWidth * 0.5f;

	const int32 Base = Verts.Num();
	Verts.Add(Center + FVector(-HW, -HW, 0.f));  // 0
	Verts.Add(Center + FVector( HW, -HW, 0.f));  // 1
	Verts.Add(Center + FVector( HW,  HW, 0.f));  // 2
	Verts.Add(Center + FVector(-HW,  HW, 0.f));  // 3

	Tris.Add(Base + 0); Tris.Add(Base + 2); Tris.Add(Base + 1);
	Tris.Add(Base + 0); Tris.Add(Base + 3); Tris.Add(Base + 2);
}

void ARoadGenerator::BuildRoadMesh()
{
	TArray<FVector>  Verts;
	TArray<int32>    Tris;
	TArray<FVector>  Normals;
	TArray<FVector2D> UVs;
	TArray<FColor>   Colors;
	TArray<FProcMeshTangent> Tangents;

	const FVector Origin = GetActorLocation();

	// Segments horizontaux (i → i+1, même j)
	for (int32 j = 0; j <= GridHeight; ++j)
		for (int32 i = 0; i < GridWidth; ++i)
			AddRoadSegment(Verts, Tris, Nodes[NodeIndex(i, j)].Location - Origin,
			                            Nodes[NodeIndex(i + 1, j)].Location - Origin);

	// Segments verticaux (j → j+1, même i)
	for (int32 i = 0; i <= GridWidth; ++i)
		for (int32 j = 0; j < GridHeight; ++j)
			AddRoadSegment(Verts, Tris, Nodes[NodeIndex(i, j)].Location - Origin,
			                            Nodes[NodeIndex(i, j + 1)].Location - Origin);

	// Carrés d'intersection à chaque nœud
	for (const FRoadNode& Node : Nodes)
		AddIntersectionSquare(Verts, Tris, Node.Location - Origin);

	// Normales et UVs simples
	Normals.Init(FVector::UpVector, Verts.Num());
	UVs.Init(FVector2D::ZeroVector, Verts.Num());

	UE_LOG(LogTemp, Warning, TEXT("[RoadGenerator] BuildRoadMesh — Verts: %d  Tris: %d  Nodes: %d"),
		Verts.Num(), Tris.Num() / 3, Nodes.Num());

	if (Verts.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[RoadGenerator] ERREUR : aucun vertex généré — grille vide ?"));
		return;
	}

	RoadMesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, Tangents, /*bCreateCollision=*/false);

	if (RoadMaterial)
		RoadMesh->SetMaterial(0, RoadMaterial);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. SpawnWaypoints
// ─────────────────────────────────────────────────────────────────────────────

void ARoadGenerator::SpawnWaypoints()
{
	SpawnedWaypoints.Reset();
	SpawnedWaypoints.SetNum(Nodes.Num());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 idx = 0; idx < Nodes.Num(); ++idx)
	{
		FTransform T(FRotator::ZeroRotator, Nodes[idx].Location);
		AActor* WP = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), T, Params);
		SpawnedWaypoints[idx] = WP;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. SpawnTrafficLights
// ─────────────────────────────────────────────────────────────────────────────

void ARoadGenerator::SpawnTrafficLights()
{
	if (!TrafficLightClass || !TrafficLightManagerClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RoadGenerator] TrafficLightClass ou TrafficLightManagerClass non assigné — feux ignorés."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 LightCount = 0;
	const int32 Cols = GridWidth  + 1;
	const int32 Rows = GridHeight + 1;

	// Tous les nœuds : coins exclus, bords = 1 feu, intérieurs = 2 feux
	for (int32 j = 0; j < Rows; ++j)
	{
		for (int32 i = 0; i < Cols; ++i)
		{
			// Route traversante H = nœud non-extrémité horizontalement
			const bool bHasHorizontal = (i > 0) && (i < GridWidth);
			// Route traversante V = nœud non-extrémité verticalement
			const bool bHasVertical   = (j > 0) && (j < GridHeight);

			// Coin (2 connexions) → pas d'intersection réelle, on passe
			if (!bHasHorizontal && !bHasVertical) continue;

			const FVector Center = Nodes[NodeIndex(i, j)].Location;
			const float   Offset = RoadWidth * 0.5f + 30.f;

			ATrafficLight* LightX = nullptr;
			ATrafficLight* LightY = nullptr;

			// Feu axe X (bloque le flux horizontal) — Yaw = 180°
			if (bHasHorizontal)
			{
				FTransform TX(FRotator(0.f, 180.f, 0.f),
				              Center + FVector(0.f, -Offset, 80.f));
				LightX = GetWorld()->SpawnActor<ATrafficLight>(TrafficLightClass, TX, Params);
			}

			// Feu axe Y (bloque le flux vertical) — Yaw = 90°
			if (bHasVertical)
			{
				FTransform TY(FRotator(0.f, 90.f, 0.f),
				              Center + FVector(Offset, 0.f, 80.f));
				LightY = GetWorld()->SpawnActor<ATrafficLight>(TrafficLightClass, TY, Params);
			}

			// Manager coordonnant les feux (GroupB vide = OK pour les bords)
			FTransform TM(FRotator::ZeroRotator, Center);
			ATrafficLightManager* Manager = GetWorld()->SpawnActor<ATrafficLightManager>(
				TrafficLightManagerClass, TM, Params);

			if (Manager)
			{
				if (LightX) Manager->GroupA.Add(LightX);
				if (LightY) Manager->GroupB.Add(LightY);
			}

			++LightCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[RoadGenerator] Feux: %d intersections"), LightCount);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. AssignVehicleRoutes
// ─────────────────────────────────────────────────────────────────────────────

int32 ARoadGenerator::FindNearestWaypointIndex(AGOAPVehicle* Vehicle,
                                                const TArray<AActor*>& Route) const
{
	const FVector VLoc = Vehicle->GetActorLocation();
	int32   Best      = 0;
	float   BestDist  = FLT_MAX;

	for (int32 k = 0; k < Route.Num(); ++k)
	{
		if (!Route[k]) continue;
		const float D = FVector::DistSquared(VLoc, Route[k]->GetActorLocation());
		if (D < BestDist) { BestDist = D; Best = k; }
	}
	return Best;
}

void ARoadGenerator::AssignVehicleRoutes()
{
	TArray<AActor*> Route = GetPerimeterRoute();
	if (Route.IsEmpty()) return;

	for (TActorIterator<AGOAPVehicle> It(GetWorld()); It; ++It)
	{
		AGOAPVehicle* V = *It;
		V->Waypoints = Route;
		V->CurrentWaypointIndex = FindNearestWaypointIndex(V, Route);
	}
}

void ARoadGenerator::ForceAssignRoutes()
{
	AssignVehicleRoutes();
}

// ─────────────────────────────────────────────────────────────────────────────
// Routes nommées
// ─────────────────────────────────────────────────────────────────────────────

TArray<AActor*> ARoadGenerator::GetPerimeterRoute() const
{
	TArray<AActor*> Route;
	if (SpawnedWaypoints.IsEmpty()) return Route;

	// Bas : j=0, i = 0 → GridWidth
	for (int32 i = 0; i <= GridWidth; ++i)
		Route.Add(SpawnedWaypoints[NodeIndex(i, 0)]);

	// Droite : i=GridWidth, j = 1 → GridHeight
	for (int32 j = 1; j <= GridHeight; ++j)
		Route.Add(SpawnedWaypoints[NodeIndex(GridWidth, j)]);

	// Haut : j=GridHeight, i = GridWidth-1 → 0
	for (int32 i = GridWidth - 1; i >= 0; --i)
		Route.Add(SpawnedWaypoints[NodeIndex(i, GridHeight)]);

	// Gauche : i=0, j = GridHeight-1 → 1
	for (int32 j = GridHeight - 1; j >= 1; --j)
		Route.Add(SpawnedWaypoints[NodeIndex(0, j)]);

	return Route;
}

TArray<AActor*> ARoadGenerator::GetRowRoute(int32 j) const
{
	TArray<AActor*> Route;
	if (SpawnedWaypoints.IsEmpty() || j < 0 || j > GridHeight) return Route;

	for (int32 i = 0; i <= GridWidth; ++i)
		Route.Add(SpawnedWaypoints[NodeIndex(i, j)]);

	return Route;
}

TArray<AActor*> ARoadGenerator::GetColumnRoute(int32 i) const
{
	TArray<AActor*> Route;
	if (SpawnedWaypoints.IsEmpty() || i < 0 || i > GridWidth) return Route;

	for (int32 j = 0; j <= GridHeight; ++j)
		Route.Add(SpawnedWaypoints[NodeIndex(i, j)]);

	return Route;
}
