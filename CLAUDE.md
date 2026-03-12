# GOAP - Projet Portfolio UE5.2 C++

## Contexte
Projet portfolio démontrant une IA GOAP (Goal-Oriented Action Planning) appliquée à des véhicules
dans Unreal Engine 5.2. Physique arcade custom (APawn), sans ChaosVehicles.

## Environnement
- **Moteur** : Unreal Engine 5.2 (`C:\Program Files\Epic Games\UE_5.2`)
- **IDE** : Visual Studio 2022
- **Projet** : `C:\Users\Thierry\Documents\Unreal Projects\Goap\Goap.uproject`
- **Module** : `Goap` — macro API : `GOAP_API`
- **Machine** : RTX 2070 Super (8GB VRAM), 32GB RAM, Windows 11

## Commandes de build

```bash
# Compiler le projet (Development Editor)
"C:\Program Files\Epic Games\UE_5.2\Engine\Build\BatchFiles\Build.bat" GoapEditor Win64 Development "C:\Users\Thierry\Documents\Unreal Projects\Goap\Goap.uproject" -WaitMutex -FromMsBuild

# Regénérer les fichiers VS (si ajout de fichiers .h/.cpp)
"C:\Program Files\Epic Games\UE_5.2\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="C:\Users\Thierry\Documents\Unreal Projects\Goap\Goap.uproject" -game -rocket -progress
```

## Logs runtime UE
```
C:\Users\Thierry\AppData\Local\UnrealEngine\5.2\Saved\Logs\
```

## Structure source

```
Source/Goap/
├── Goap.Build.cs               ← Dépendances : Core, CoreUObject, Engine, InputCore,
│                                   EnhancedInput, AIModule, ProceduralMeshComponent
├── Goap.h / Goap.cpp           ← Module entry point (template, ne pas modifier)
├── GoapGameMode.h/.cpp         ← GameMode de base (template, intact)
├── GoapCharacter.h/.cpp        ← Character de base (template, intact)
│
├── GOAP/
│   ├── WorldState.h            ← struct FWorldState : TMap<FName,bool>
│   │                               Méthodes : Satisfies(), DistanceTo(), Apply()
│   ├── GOAPAction.h/.cpp       ← UObject abstrait : Preconditions/Effects/Cost
│   │                               Interface : Activate(), Tick(), IsFinished(), Abort()
│   └── GOAPPlanner.h/.cpp      ← class FGOAPPlanner — static Plan()
│                                   Algorithme A* forward, max 500 itérations
│
├── Road/
│   └── RoadGenerator.h/.cpp    ← ARoadGenerator : grille procédurale N×M
│                                   UProceduralMeshComponent, FRoadNode, spawn waypoints
│                                   Spawn feux aux coins des intersections (conduite à droite)
│                                     SW(Yaw=0°)+NE(Yaw=180°) = GroupA horizontal
│                                     NW(Yaw=270°)+SE(Yaw=90°) = GroupB vertical
│                                   AssignVehicleRoutes() : pool périmètre+lignes+colonnes
│                                     round-robin par véhicule, PickRandomDestination()
│                                     puis RebuildTrafficLightCache()+Replan()
│                                   ForceAssignRoutes() — public, appelable depuis éditeur
│                                   bShowStopZones : debug sphères + flèches orientation feux
│                                   GetPerimeterRoute(), GetRowRoute(), GetColumnRoute()
│
├── Traffic/
│   ├── TrafficLight.h/.cpp     ← ATrafficLight : Red/Yellow/Green, IsBlockingTraffic()
│   └── TrafficLightManager.h/.cpp ← ATrafficLightManager : GroupA/GroupB, 4 phases
│                                   GreenDuration=5s, YellowDuration=2s
│
├── Vehicle/
│   ├── GOAPVehicle.h/.cpp      ← APawn custom (physique arcade manuelle)
│   │                               UStaticMeshComponent, SetThrottle/SetSteering
│   │                               TArray<AActor*> Waypoints, float WaypointAcceptRadius
│   │                               CurrentWaypointIndex, DestinationWaypointIndex
│   │                               AdvanceTowardDestination() — avance vers la destination (circulaire)
│   │                               PickRandomDestination() — choisit destination ≠ courant
│   │                               HasReachedDestination() — CurrentWP == DestinationWP
│   │                               GetCurrentWaypointLocation() — position du WP courant
│   │                               Tick : FInterpTo vitesse + AddActorWorldOffset (sweep=true)
│   │                               Debug : WP courant=jaune, destination=vert, ligne véhicule→dest
│   ├── DriveToWaypointAction.h/.cpp
│   │                           ← pre={CanDrive=true}, eff={AtWaypoint=true}, Cost=1
│   │                               Steering via CrossProduct + LateralOffset (conduite à droite)
│   ├── WaitAtWaypointAction.h/.cpp
│   │                           ← pre={AtWaypoint=true}, eff={Resting=true}
│   │                               Attend WaitDuration secondes, puis appelle AdvanceTowardDestination()
│   └── WaitForGreenLightAction.h/.cpp
│                               ← pre={}, eff={CanDrive=true}, Cost=5
│                                   CheckProcedural : FindNearestBlockingLight(DetectionRadius=1500cm)
│
└── Controllers/
    └── GOAPVehicleController.h/.cpp
                                ← AAIController, Goal = {Resting=true}
                                    OnPossess → CanDrive=true init, cache feux, PickRandomDestination
                                                si waypoints déjà assignés, BuildActions+Replan
                                    Tick : CheckTrafficLightInterrupt() + TickCurrentAction()
                                    Plan épuisé → CanDrive=!HasNearbyRedLight() → Replan
                                    HasNearbyRedLight() : angle + ForwardDist(min=500cm) + orientation feu
                                    TrafficLightDetectionRadius=1500cm, TrafficLightMinForwardDistance=500cm
                                    RebuildTrafficLightCache() — public, appelé par RoadGenerator
```

## Flux GOAP — cycle destination aléatoire

```
OnPossess / AssignVehicleRoutes → PickRandomDestination() → DestinationWaypointIndex
        ↓
WorldState={CanDrive=true}  +  Goal={Resting=true}
        ↓
Plan = [DriveToWaypointAction, WaitAtWaypointAction]
        ↓
DriveToWaypoint  →  WorldState={AtWaypoint=true}
        ↓
WaitAtWaypoint (2s)  →  AdvanceTowardDestination()  →  WorldState={AtWaypoint=true, Resting=true}
        ↓
Plan épuisé → reset WorldState={CanDrive=!HasNearbyRedLight()} → Replan
        ↓
WaitAtWaypoint vérifie HasReachedDestination() → PickRandomDestination() → boucle …

Interruption feu rouge :
  CheckTrafficLightInterrupt() détecte feu → CanDrive=false → Abort DriveToWaypoint → Replan
  Plan = [WaitForGreenLightAction, DriveToWaypointAction, WaitAtWaypointAction]
  Feu vert → CanDrive=true → WaitForGreenLight::IsFinished() → suite normale
```

## Conventions de code

- **Pas de ChaosVehicles** — physique 100% manuelle via `AddActorWorldOffset`
- **C++ pur pour la logique IA** — pas de Blueprint pour GOAP/actions/controller
- Blueprints uniquement pour : assignation de mesh, placement éditeur, waypoints
- Préfixes UE standards : `F` struct, `U` UObject, `A` Actor, `I` Interface
- Inclure le PCH via `#include "CoreMinimal.h"` en premier dans les `.cpp`
- Toujours ajouter le header généré `#include "NomDuFichier.generated.h"` en dernier include

## Plugins actifs

| Plugin | Statut |
|--------|--------|
| ModelingToolsEditorMode | Activé |
| ChaosVehiclesPlugin | Présent dans .uproject, NON utilisé en code |

## Setup éditeur (à faire une seule fois)

1. Clic droit `Goap.uproject` → "Generate Visual Studio project files"
2. Ouvrir `Goap.sln` dans VS2022 → Build `Development Editor`
3. Dans UE Editor : créer `BP_GOAPVehicle` (Blueprint depuis `AGOAPVehicle`)
4. Assigner un StaticMesh voiture dans `BP_GOAPVehicle`
5. Placer des `AActor` vides dans le niveau → les assigner dans le tableau `Waypoints`
6. Assigner `AGOAPVehicleController` comme AIController dans `BP_GOAPVehicle`

## Fichiers à ne pas modifier sans raison

- `Goap.h`, `Goap.cpp`, `GoapGameMode.*`, `GoapCharacter.*` — template UE intact
- `Source/Goap.Target.cs`, `Source/GoapEditor.Target.cs` — targets de build
