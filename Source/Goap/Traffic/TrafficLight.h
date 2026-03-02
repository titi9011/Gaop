#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficLight.generated.h"

UENUM(BlueprintType)
enum class ETrafficLightState : uint8
{
	Red    UMETA(DisplayName="Red"),
	Yellow UMETA(DisplayName="Yellow"),
	Green  UMETA(DisplayName="Green")
};

/**
 * Feu de circulation : Rouge / Jaune / Vert.
 * Contrôlé par ATrafficLightManager.
 * IsBlockingTraffic() == true sur Rouge et Jaune.
 *
 * Visuel : assigner les sphères du Blueprint à LightRed / LightYellow / LightGreen.
 * Seule la sphère correspondant à l'état actif est visible.
 */
UCLASS()
class GOAP_API ATrafficLight : public AActor
{
	GENERATED_BODY()

public:
	ATrafficLight();

	void SetState(ETrafficLightState NewState);
	ETrafficLightState GetState() const { return CurrentState; }

	/** Vrai si les voitures doivent s'arrêter (Rouge ou Jaune) */
	bool IsBlockingTraffic() const;

	UPROPERTY(VisibleAnywhere) USceneComponent*      Root;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* LightRed;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* LightYellow;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* LightGreen;

protected:
	virtual void BeginPlay() override;

private:
	ETrafficLightState CurrentState = ETrafficLightState::Red;

	void RefreshVisibility();
};
