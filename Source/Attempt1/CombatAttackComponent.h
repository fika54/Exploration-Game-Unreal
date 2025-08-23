#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatInterface.h"
#include "CombatAttackComponent.generated.h"

class UAnimInstance;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDealtDamageSig, float, Damage, FVector, ImpactPoint);

UCLASS(ClassGroup = (Combat), Blueprintable, meta = (BlueprintSpawnableComponent))
class ATTEMPT1_API UCombatAttackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCombatAttackComponent();

    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnDealtDamageSig OnDealtDamage;

    UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
    virtual void PrimaryAttack() {}

    UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
    virtual void SecondaryAttack() {}

protected:
    virtual void BeginPlay() override;

protected:
    UPROPERTY() ACharacter* OwnerChar = nullptr;
    UPROPERTY() UAnimInstance* AnimInst = nullptr;
};
