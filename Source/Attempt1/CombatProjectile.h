#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CombatProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class ATTEMPT1_API ACombatProjectile : public AActor
{
    GENERATED_BODY()

public:
    ACombatProjectile();
    void InitProjectile(AActor* InShooter, float InDamage, float InRange);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleDefaultsOnly, Category = "Projectile")
    USphereComponent* Collision;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    UProjectileMovementComponent* Movement;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float InitialSpeed = 3000.f;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float LifeSeconds = 3.f;

private:
    UFUNCTION()
    void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    TWeakObjectPtr<AActor> Shooter;
    float Damage = 10.f;
};
