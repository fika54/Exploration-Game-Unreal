#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatInterface.h" // for FDamageResult
#include "CombatDefenceComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class ATTEMPT1_API UCombatDefenceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCombatDefenceComponent();

    // HP
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defence|HP") float MaxHP = 5.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Defence|HP") float CurrentHP = 5.f;

    // Block/Parry
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defence|Block", meta = (ClampMin = 0))
    float ParryWindowSeconds = 0.25f;

    // 0.8 => 80% reduction (20% chip)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defence|Block", meta = (ClampMin = 0, ClampMax = 1))
    float BlockDamageReduction = 0.8f;

    // Dodge
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defence|Dodge")
    float DodgeIFrameSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defence|Dodge")
    float DodgeImpulse = 1200.f;

    // API
    void StartBlock();
    void EndBlock();
    void Dodge(const FVector& WorldDir);

    float GetParryWindow() const { return ParryWindowSeconds; }

    FDamageResult HandleIncomingAttack(AActor* Attacker, float IncomingDamage,
        const FVector& ImpactPoint, const FVector& ImpulseDir);

    void ApplyRawDamage(float Damage, AActor* Causer, const FVector& ImpactPoint, const FVector& ImpulseDir);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() ACharacter* OwnerChar = nullptr;

    bool  bIsBlocking = false;
    bool  bIsDodging = false;
    double BlockStartTime = 0.0;
    double DodgeEndTime = 0.0;

    void ApplyImpulseToCharacter(const FVector& Impulse) const;
    void Die();
};
