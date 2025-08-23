#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "CombatAttackComponent.h"
#include "CombatMeleeComponent.generated.h"

class UAnimMontage;

UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class ATTEMPT1_API UCombatMeleeComponent : public UCombatAttackComponent
{
    GENERATED_BODY()

public:
    UCombatMeleeComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Montage")
    UAnimMontage* LightComboMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Montage")
    TArray<FName> LightComboSections; // Light_1, Light_2, Light_3

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Montage")
    UAnimMontage* HeavyMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = 0, ClampMax = 1))
    float ComboInputCacheTolerance = 0.45f;

    /** Time at which an attack button was last pressed */
    float CachedAttackInputTime = 0.0f;

    /** Index of the current stage of the melee attack combo */
    int32 ComboCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Trace")
    FAttackTraceSpec LightTraceSpec;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Trace")
    FAttackTraceSpec HeavyTraceSpec;

    /** Attack montage ended delegate */
    FOnMontageEnded OnAttackMontageEnded;

    

    virtual void PrimaryAttack() override;   // light combo
    virtual void SecondaryAttack() override; // heavy

    UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
    void PerformAttackTrace(const FAttackTraceSpec& Spec);

    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void Notify_CheckComboWindow();

private:
    bool   bIsAttacking = false;
    bool   bIsChargingAttack = false;
    int32  ComboIndex = 0;
    double CachedLightPressTime = -1000.0;

    FOnMontageEnded OnMontageEndedDelegate;



    void PlayLightStart();
    void PlayLightSection(int32 Index);
    void PlayHeavy();
    void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FVector GetTraceStartWS(const FAttackTraceSpec& Spec) const;
    void ApplyImpulseToCharacter(ACharacter* Char, const FVector& Impulse) const;
};
