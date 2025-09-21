#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h" 
#include "CombatInterface.generated.h"

// =====================
// Data Types
// =====================

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Light,   // primary (M1)
    Heavy    // secondary (M2)
};

USTRUCT(BlueprintType)
struct FAttackTraceSpec
{
    GENERATED_BODY()

    // Trace shape and reach
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = 0, Units = "cm"))
    float Distance = 75.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace", meta = (ClampMin = 0, Units = "cm"))
    float Radius = 60.f;

    // Damage and impulses
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float Damage = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float KnockbackImpulse = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
    float LaunchImpulse = 250.f;

    // Socket/Bone to start the trace from (for fists or weapon)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    bool bUseSocket = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    FName SocketOrBone = FName("hand_r");

    // Debug draw
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDebug = false;
};

USTRUCT(BlueprintType)
struct FDamageResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bBlocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bParried = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ActualDamage = 0.f;
};


// =====================
// Interface
// =====================

UINTERFACE(Blueprintable)
class ATTEMPT1_API UCombatInterface : public UInterface
{
    GENERATED_BODY()
};

class ATTEMPT1_API ICombatInterface
{
    GENERATED_BODY()
public:
    // High-level attack requests (input-level)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Attack")
    void TryAttack(EAttackType Type);

    // Per-hit trace execution (usually called from AnimNotifies)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Attack")
    void PerformAttackTrace(const FAttackTraceSpec& Spec);

    // Callback when we dealt damage to a target (for VFX/SFX/UI hooks)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Attack")
    void OnDealtDamage(AActor* Target, float Damage, const FVector& ImpactPoint);

    // Defence API
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Defence")
    void StartBlock();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Defence")
    void EndBlock();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Defence")
    float GetParryWindowSeconds() const;

    // Called on the target when an attack arrives; returns result after block/parry reductions
    UFUNCTION(BlueprintNativeEvent, Category = "Combat|Defence", meta = (BlueprintInternalUseOnly = "true"))
    FDamageResult HandleIncomingAttack(AActor* Attacker, float IncomingDamage,
        const FVector& ImpactPoint, const FVector& ImpulseDir);

    // Fallback raw damage application for non-interface actors
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Defence")
    void ApplyRawDamage(float Damage, AActor* Causer, const FVector& ImpactPoint, const FVector& ImpulseDir);

};
