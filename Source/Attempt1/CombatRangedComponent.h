#pragma once
#include "CoreMinimal.h"
#include "CombatAttackComponent.h"
#include "CombatRangedComponent.generated.h"

class ACombatProjectile;

USTRUCT(BlueprintType)
struct FRangedAttackSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") float Damage = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") float Range = 10000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") float SpreadDegrees = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") FName  MuzzleSocket = "Muzzle";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged") bool   bUseHitscan = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (EditCondition = "!bUseHitscan"))
    TSubclassOf<ACombatProjectile> ProjectileClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug") bool bDebug = false;
};

UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class ATTEMPT1_API UCombatRangedComponent : public UCombatAttackComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ranged")
    FRangedAttackSpec PrimarySpec;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Ranged")
    FRangedAttackSpec SecondarySpec;

    virtual void PrimaryAttack() override;
    virtual void SecondaryAttack() override;

private:
    void Fire(const FRangedAttackSpec& Spec);
    bool Hitscan(const FRangedAttackSpec& Spec);
    void SpawnProjectile(const FRangedAttackSpec& Spec);
};
