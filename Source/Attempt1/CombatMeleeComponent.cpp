#include "CombatMeleeComponent.h"
#include "CombatInterface.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UCombatMeleeComponent::UCombatMeleeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    LightTraceSpec.Distance = 75.f;
    LightTraceSpec.Radius = 60.f;
    LightTraceSpec.Damage = 1.f;
    LightTraceSpec.SocketOrBone = FName("hand_r");

    HeavyTraceSpec = LightTraceSpec;
    HeavyTraceSpec.Distance = 90.f;
    HeavyTraceSpec.Radius = 75.f;
    HeavyTraceSpec.Damage = 2.5f;
    HeavyTraceSpec.SocketOrBone = FName("weapon_socket");

    OnMontageEndedDelegate.BindUObject(this, &UCombatMeleeComponent::AttackMontageEnded);
}

void UCombatMeleeComponent::PrimaryAttack()
{
    if (!OwnerChar || !AnimInst) return;

    const double Now = GetWorld()->GetTimeSeconds();

    if (bIsAttacking)
    {
        // Buffer the press while a section is playing (do NOT play the montage again)
        CachedLightPressTime = Now;
        return;
    }

    PlayLightStart();
}
    
void UCombatMeleeComponent::SecondaryAttack()
{
    PlayHeavy();
}

void UCombatMeleeComponent::PlayLightStart()
{
    if (!AnimInst || !LightComboMontage || LightComboSections.Num() == 0) return;

    bIsAttacking = true;
    ComboIndex = 0;

    // play the attack montage
    if (UAnimInstance* AnimInstance = AnimInst)
    {
        const float MontageLength = AnimInstance->Montage_Play(LightComboMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

        // subscribe to montage completed and interrupted events
        if (MontageLength > 0.0f)
        {
            // set the end delegate for the montage
            AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded, LightComboMontage);
        }
    }

}

// NOTE: Keep this for emergencies (forced cuts) or initial jumps only.
// For smooth chaining we will SCHEDULE the next section in Notify_CheckComboWindow instead.
void UCombatMeleeComponent::PlayLightSection(int32 Index)
{
    if (!AnimInst || !LightComboMontage || !LightComboSections.IsValidIndex(Index)) return;
    // are we playing a non-charge attack animation?
    if (bIsAttacking && !bIsChargingAttack)
    {
        // is the last attack input not stale?
        if (GetWorld()->GetTimeSeconds() - CachedAttackInputTime <= ComboInputCacheTolerance)
        {
            // consume the attack input so we don't accidentally trigger it twice
            CachedLightPressTime = 0.0f;

            // increase the combo counter
            ++ComboCount;

            // do we still have a combo section to play?
            if (ComboCount < LightComboSections.Num())
            {
                // jump to the next combo section
                if (UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance())
                {
                    AnimInstance->Montage_JumpToSection(LightComboSections[ComboCount], LightComboMontage);
                }
            }
        }
    }
}

void UCombatMeleeComponent::PlayHeavy()
{
    if (!AnimInst || !HeavyMontage) return;

    bIsAttacking = true;
    AnimInst->Montage_Play(HeavyMontage, 1.f);
    AnimInst->Montage_SetEndDelegate(OnMontageEndedDelegate, HeavyMontage);
}

// ======= Smooth combo window check =======
// Call this from a notify placed LATE in each section (last ~15–25%).
void UCombatMeleeComponent::Notify_CheckComboWindow()
{
    if (!bIsAttacking || !AnimInst || !LightComboMontage) return;

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - CachedLightPressTime <= ComboInputCacheTolerance)
    {
        CachedLightPressTime = -1000.0; // consume

        const int32 Curr = ComboIndex;
        const int32 Next = Curr + 1;
        if (LightComboSections.IsValidIndex(Curr) && LightComboSections.IsValidIndex(Next))
        {
            // Seamless: tell UE what comes next while still inside Curr
            AnimInst->Montage_SetNextSection(LightComboSections[Curr], LightComboSections[Next], LightComboMontage);
            ComboIndex = Next;
        }
    }
}

void UCombatMeleeComponent::PerformAttackTrace(const FAttackTraceSpec& Spec)
{
    if (!OwnerChar) return;

    const FVector Start = GetTraceStartWS(Spec);
    const FVector End = Start + OwnerChar->GetActorForwardVector() * Spec.Distance;

    TArray<FHitResult> Hits;

    FCollisionObjectQueryParams Obj;
    Obj.AddObjectTypesToQuery(ECC_Pawn);
    Obj.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape Shape = FCollisionShape::MakeSphere(Spec.Radius);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChar);

    const bool bHit = GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, Shape, Params);

    if (Spec.bDebug)
    {
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Start, Spec.Radius, 12, FLinearColor::Blue, 2.f, 1.f);
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), End, Spec.Radius, 12, FLinearColor::Green, 2.f, 1.f);
        UKismetSystemLibrary::DrawDebugLine(GetWorld(), Start, End, FLinearColor::White, 2.f, 1.f);
    }

    if (!bHit) return;

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!Target || Target == OwnerChar) continue;

        const FVector ImpulseDir = (-Hit.ImpactNormal).GetSafeNormal();

        if (Target->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
        {
            const FDamageResult Result =
                ICombatInterface::Execute_HandleIncomingAttack(Target, OwnerChar, Spec.Damage, Hit.ImpactPoint, ImpulseDir);

            if (Result.ActualDamage > 0.f)
            {
                OnDealtDamage.Broadcast(Result.ActualDamage, Hit.ImpactPoint);
                ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Result.ActualDamage, Hit.ImpactPoint);
            }
        }
        else
        {
            ICombatInterface::Execute_ApplyRawDamage(Target, Spec.Damage, OwnerChar, Hit.ImpactPoint, ImpulseDir);
            OnDealtDamage.Broadcast(Spec.Damage, Hit.ImpactPoint);
            ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Spec.Damage, Hit.ImpactPoint);
        }
    }
}

void UCombatMeleeComponent::AttackMontageEnded(UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
{
    bIsAttacking = false;
    ComboIndex = 0;
    CachedLightPressTime = -1000.0;
}

FVector UCombatMeleeComponent::GetTraceStartWS(const FAttackTraceSpec& Spec) const
{
    if (!OwnerChar || !OwnerChar->GetMesh())
        return OwnerChar ? OwnerChar->GetActorLocation() : FVector::ZeroVector;

    return Spec.bUseSocket
        ? OwnerChar->GetMesh()->GetSocketLocation(Spec.SocketOrBone)
        : OwnerChar->GetMesh()->GetBoneLocation(Spec.SocketOrBone);
}

void UCombatMeleeComponent::ApplyImpulseToCharacter(ACharacter* Char, const FVector& Impulse) const
{
    if (!Char) return;
    if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
    {
        Move->AddImpulse(Impulse, true);
    }
}
