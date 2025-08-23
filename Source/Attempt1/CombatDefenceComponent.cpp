#include "CombatDefenceComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCombatDefenceComponent::UCombatDefenceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatDefenceComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerChar = Cast<ACharacter>(GetOwner());
    CurrentHP = MaxHP;
}

void UCombatDefenceComponent::StartBlock()
{
    bIsBlocking = true;
    BlockStartTime = GetWorld()->GetTimeSeconds();
}

void UCombatDefenceComponent::EndBlock()
{
    bIsBlocking = false;
}

void UCombatDefenceComponent::Dodge(const FVector& WorldDir)
{
    if (!OwnerChar) return;
    const FVector Dir = WorldDir.IsNearlyZero() ? OwnerChar->GetActorForwardVector() : WorldDir.GetSafeNormal();
    OwnerChar->LaunchCharacter(Dir * DodgeImpulse, true, true);
    bIsDodging = true;
    DodgeEndTime = GetWorld()->GetTimeSeconds() + DodgeIFrameSeconds;
}

FDamageResult UCombatDefenceComponent::HandleIncomingAttack(AActor* /*Attacker*/, float IncomingDamage,
    const FVector& /*ImpactPoint*/, const FVector& ImpulseDir)
{
    FDamageResult Out;
    const double Now = GetWorld()->GetTimeSeconds();

    if (bIsDodging && Now <= DodgeEndTime)
    {
        Out.ActualDamage = 0.f;
        return Out;
    }

    if (bIsBlocking)
    {
        const bool bParry = (Now - BlockStartTime) <= ParryWindowSeconds;
        if (bParry)
        {
            Out.bParried = true;
            Out.ActualDamage = 0.f;
            return Out;
        }
        Out.bBlocked = true;
        Out.ActualDamage = FMath::Max(0.f, IncomingDamage * (1.f - BlockDamageReduction));
    }
    else
    {
        Out.ActualDamage = IncomingDamage;
    }

    CurrentHP -= Out.ActualDamage;

    if (Out.ActualDamage > 0.f)
    {
        const FVector Impulse = ImpulseDir * 250.f + FVector::UpVector * 200.f;
        ApplyImpulseToCharacter(Impulse);
    }

    if (CurrentHP <= 0.f)
    {
        Die();
    }

    if (bIsDodging && Now > DodgeEndTime)
    {
        bIsDodging = false;
    }

    return Out;
}

void UCombatDefenceComponent::ApplyRawDamage(float Damage, AActor* /*Causer*/, const FVector& /*ImpactPoint*/, const FVector& ImpulseDir)
{
    CurrentHP -= Damage;

    if (Damage > 0.f)
    {
        const FVector Impulse = ImpulseDir * 200.f + FVector::UpVector * 150.f;
        ApplyImpulseToCharacter(Impulse);
    }

    if (CurrentHP <= 0.f)
    {
        Die();
    }
}

void UCombatDefenceComponent::ApplyImpulseToCharacter(const FVector& Impulse) const
{
    if (OwnerChar)
    {
        if (UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement())
        {
            Move->AddImpulse(Impulse, true);
        }
    }
}

void UCombatDefenceComponent::Die()
{
    if (OwnerChar)
    {
        if (UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement())
        {
            Move->DisableMovement();
        }
    }
}
