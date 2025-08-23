#include "CombatAttackComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

UCombatAttackComponent::UCombatAttackComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatAttackComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerChar = Cast<ACharacter>(GetOwner());
    if (OwnerChar && OwnerChar->GetMesh())
    {
        AnimInst = OwnerChar->GetMesh()->GetAnimInstance();
    }
}
