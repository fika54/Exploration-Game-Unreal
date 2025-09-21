#include "ProtagonistCharacter.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"

#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"


#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "CombatDefenceComponent.h"
#include "CombatMeleeComponent.h"
#include "CombatRangedComponent.h"
#include "CombatAttackComponent.h"

DEFINE_LOG_CATEGORY(LogProtagonistCharacter);

AProtagonistCharacter::AProtagonistCharacter()
{
    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Controller rotation -> camera only
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    //sprint testing
    WalkSpeed = 500.0f;
    SprintSpeed = 800.0f;
    SpeedInterpolationRate = 20.0f; // Higher = faster response

    // Initialize TargetSpeed to walk speed
    TargetSpeed = WalkSpeed;

    // Movement tuning
    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = true;
    Move->RotationRate = FRotator(0.f, 500.f, 0.f);
    Move->JumpZVelocity = 500.f;
    Move->AirControl = 0.35f;
    Move->MaxWalkSpeed = 500.f;
    Move->MinAnalogWalkSpeed = 20.f;
    Move->BrakingDecelerationWalking = 2000.f;
    Move->BrakingDecelerationFalling = 1500.f;

    // Camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->bUsePawnControlRotation = true;

    // Follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Combat components
    Defence = CreateDefaultSubobject<UCombatDefenceComponent>(TEXT("Defence"));
    Melee = CreateDefaultSubobject<UCombatMeleeComponent>(TEXT("Melee"));
    Ranged = CreateDefaultSubobject<UCombatRangedComponent>(TEXT("Ranged"));


    ActiveAttack = Melee; // default
}

void AProtagonistCharacter::BeginPlay()
{
    Super::BeginPlay();
    EquipDefaultWeapon(true);
    SetCameraStrafeMode(true, /*bInstant*/true);
    AddDefaultIMC_Local();
}

void AProtagonistCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Smoothly interpolate MaxWalkSpeed to TargetSpeed
    if (GetCharacterMovement())
    {
        float CurrentSpeed = GetCharacterMovement()->MaxWalkSpeed;
        float NewSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, SpeedInterpolationRate);
        GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
    }
}

void AProtagonistCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        if (MoveAction)      EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Move);
        if (MouseLookAction) EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Look);
        if (LookAction)      EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Look);

        if (PrimaryAttackAction)   EIC->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnPrimary);


        if (SecondaryAttackAction) 
        { 
            EIC->BindAction(SecondaryAttackAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnSecondaryStart); 
            EIC->BindAction(SecondaryAttackAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::OnSecondaryRelease);
        }

        if (ToggleStrafeAction)
        {
            EIC->BindAction(ToggleStrafeAction, ETriggerEvent::Started, this, &AProtagonistCharacter::ToggleCameraStrafeMode);
        }


        if (BlockAction)
        {
            EIC->BindAction(BlockAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnBlockStart);
            EIC->BindAction(BlockAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::OnBlockEnd);
        }

        if (DodgeAction) EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnDodgeOrSprint);

        if (ToggleMobilityAction) EIC->BindAction(ToggleMobilityAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnToggleHolster);
    }
    else
    {
        UE_LOG(LogProtagonistCharacter, Error, TEXT("%s: EnhancedInputComponent not found."), *GetNameSafe(this));
    }
}

void AProtagonistCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    DoMove(V.X, V.Y);
}

void AProtagonistCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    DoLook(V.X, V.Y);
}

void AProtagonistCharacter::DoMove(float Right, float Forward)
{
    if (Controller)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

        const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDir, Forward);
        AddMovementInput(RightDir, Right);
    }
}

void AProtagonistCharacter::DoLook(float Yaw, float Pitch)
{
    if (Controller)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void AProtagonistCharacter::DoJumpStart() { Jump(); }
void AProtagonistCharacter::DoJumpEnd() { StopJumping(); }


//void AProtagonistCharacter::EquipWeaponClass(TSubclassOf<UWeaponComponent> WeaponClass, bool bPlayMontage)
//{
//    if (!WeaponClass) return;
//
//    // If you already have a weapon, you might holster/destroy it here
//    if (ActiveWeapon)
//    {
//        ActiveWeapon->HolsterWeapon(false);
//        //ActiveWeapon->DestroyComponent();    // optional, if you want to replace
//        ActiveWeapon = nullptr;
//    }
//
//    // Create an instance of that WeaponComponent class on this character
//    ActiveWeapon = NewObject<UWeaponComponent>(this, WeaponClass);
//    if (!ActiveWeapon) return;
//
//    ActiveWeapon->RegisterComponent();       // attach & initialize
//    ActiveWeapon->EquipWeapon(bPlayMontage); // attaches to hand socket, plays unholster if set
//}

// Equip helpers
void AProtagonistCharacter::EquipMelee() { ActiveAttack = Melee; }
void AProtagonistCharacter::EquipRanged() { ActiveAttack = Ranged; }

// Combat input handlers
void AProtagonistCharacter::OnPrimary()
{
    if (bMobilityMode) {
        //code for mobility mode (should reach into the mobility mode component for it)
    }
    else {
        if (ActiveWeapon) ActiveWeapon->PrimaryAttack();
    }
    
}

void AProtagonistCharacter::OnSecondaryStart()
{
    if (bMobilityMode) {

    }
    else {
        if (ActiveWeapon) ActiveWeapon->SecondaryAttack_Start();
    }
    
}

void AProtagonistCharacter::OnSecondaryRelease()
{
    if (bMobilityMode) {

    }
    else {
        if (ActiveWeapon) ActiveWeapon->SecondaryAttack_Release();
    }
}


//switches between mobility and combat mode
void AProtagonistCharacter::OnToggleHolster()
{
    if (!ActiveWeapon) return;

    if (bMobilityMode) {
        bMobilityMode = false;
        ActiveWeapon->EquipWeapon(true);
    }
    else {
        bMobilityMode = true;
        ActiveWeapon->HolsterWeapon(true);
    }


    /*ActiveWeapon->bIsEquipped ? ActiveWeapon->HolsterWeapon(true)
        : ActiveWeapon->EquipWeapon(true);*/
}

void AProtagonistCharacter::OnBlockStart() { StartBlock_Implementation(); }
void AProtagonistCharacter::OnBlockEnd() { EndBlock_Implementation(); }

void AProtagonistCharacter::OnDodgeOrSprint()
{
    /*FVector Dir = GetActorForwardVector();
    if (Controller)
    {
        const FRotator Rot = Controller->GetControlRotation();
        Dir = FRotationMatrix(FRotator(0.f, Rot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
    }
    if (Defence) Defence->Dodge(Dir);*/

    if (bMobilityMode) {
        if (TargetSpeed > WalkSpeed) {
            TargetSpeed = WalkSpeed;
        }
        else {
            TargetSpeed = SprintSpeed;
        }
    }
    else {
        ActiveWeapon->onDodge();
    }
}


// AnimNotifies for melee hit frames
void AProtagonistCharacter::AN_MeleeLightTrace()
{
    if (Melee) Melee->PerformAttackTrace(Melee->LightTraceSpec);
}

void AProtagonistCharacter::AN_MeleeHeavyTrace()
{
    if (Melee) Melee->PerformAttackTrace(Melee->HeavyTraceSpec);
}

// ICombatInterface forwarding
void AProtagonistCharacter::TryAttack_Implementation(EAttackType Type)
{
    if (!ActiveAttack) return;
    if (Type == EAttackType::Light) ActiveAttack->PrimaryAttack();
    else                            ActiveAttack->SecondaryAttack();
}

void AProtagonistCharacter::PerformAttackTrace_Implementation(const FAttackTraceSpec& Spec)
{
    if (Melee) Melee->PerformAttackTrace(Spec);
}

void AProtagonistCharacter::StartBlock_Implementation()
{
    if (Defence) Defence->StartBlock();
}

void AProtagonistCharacter::EndBlock_Implementation()
{
    if (Defence) Defence->EndBlock();
}

float AProtagonistCharacter::GetParryWindowSeconds_Implementation() const
{
    return Defence ? Defence->GetParryWindow() : 0.25f;
}

FDamageResult AProtagonistCharacter::HandleIncomingAttack_Implementation(
    AActor* Attacker, float IncomingDamage, const FVector& ImpactPoint, const FVector& ImpulseDir)
{
    return Defence ? Defence->HandleIncomingAttack(Attacker, IncomingDamage, ImpactPoint, ImpulseDir)
        : FDamageResult{true, true, 9};
}

void AProtagonistCharacter::ApplyRawDamage_Implementation(
    float Damage, AActor* Causer, const FVector& ImpactPoint, const FVector& ImpulseDir)
{
    if (Defence) Defence->ApplyRawDamage(Damage, Causer, ImpactPoint, ImpulseDir);
}


void AProtagonistCharacter::EnableStrafing()
{
    auto* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = false;   // don't turn toward velocity
    bUseControllerRotationYaw = true;          // face ControlRotation yaw directly
    Move->bUseControllerDesiredRotation = false;
    //Move->RotationRate = FRotator(0.f, 720.f, 0.f); // not used here, but fine
    GetCameraBoom()->bUsePawnControlRotation = true;
}

void AProtagonistCharacter::EquipDefaultWeapon(bool bPlayMontage)
{
    if (DefaultWeaponClass)
    {
        EquipWeaponClass(DefaultWeaponClass, /*bHolsterPrevious*/true);
        if (ActiveWeapon && bPlayMontage)
        {
            ActiveWeapon->EquipWeapon(true);
        }
    }
}

void AProtagonistCharacter::EquipWeaponClass(TSubclassOf<UWeaponComponent> WeaponClass, bool bHolsterPrevious)
{
    // Holster & unhook previous
    if (ActiveWeapon)
    {
        if (bHolsterPrevious) ActiveWeapon->HolsterWeapon(true);
        ActiveWeapon->OnLocomotionSetChanged.RemoveAll(this);
        ActiveWeapon = nullptr;
    }
    if (!WeaponClass) return;

    // Spawn a new component owned by this character
    UWeaponComponent* NewWeapon = NewObject<UWeaponComponent>(this, WeaponClass);
    if (!NewWeapon) return;

    // Register so it ticks/works & won’t get GC’d (UPROPERTY keeps it alive too)
    NewWeapon->RegisterComponent();                               // required
    AddInstanceComponent(NewWeapon);                              // optional but good practice
    // NewWeapon attaches to mesh in its own EquipWeapon(); nothing else needed here

    ActiveWeapon = NewWeapon;
    ActiveWeapon->OnLocomotionSetChanged.AddDynamic(this, &AProtagonistCharacter::OnWeaponLocomotionChanged);

    // Auto-equip
    ActiveWeapon->EquipWeapon(true);
}

void AProtagonistCharacter::OnWeaponLocomotionChanged(EWeaponLocomotionSet NewSet)
{
    // Example: store to a variable the AnimBP reads (Blend by Enum)
    // CurrentWeaponLocomotion = NewSet;  // (make this a UPROPERTY on the character)
}

void AProtagonistCharacter::ToggleCameraStrafeMode()
{
    SetCameraStrafeMode(!bCameraStrafeMode, /*bInstant*/false);
}

void AProtagonistCharacter::SetCameraStrafeMode(bool bEnable, bool bInstant)
{
    bCameraStrafeMode = bEnable;
    bIsStrafingLocomotion = bEnable;

    UCharacterMovementComponent* Move = GetCharacterMovement();
    if (!Move) return;

    if (bEnable)
    {
        // Face the camera yaw and STRAFE
        Move->bOrientRotationToMovement = false;  // don't face velocity
        Move->bUseControllerDesiredRotation = true; // smoothly match ControlRotation
        bUseControllerRotationYaw = true;  // character derives yaw from controller
        Move->RotationRate = FRotator(0.f, 720.f, 0.f); // smoothing

        // camera still driven by controller
        if (CameraBoom) CameraBoom->bUsePawnControlRotation = true;

        if (bInstant)
        {
            FRotator CamYawOnly(0.f, GetControlRotation().Yaw, 0.f);
            SetActorRotation(CamYawOnly); // immediate alignment
        }
        // else: auto-correct happens smoothly via bUseControllerDesiredRotation + RotationRate
    }
    else
    {
        // FREE-LOOK ORBIT: camera can spin around; character faces movement
        bUseControllerRotationYaw = false; // camera yaw no longer turns the pawn
        Move->bUseControllerDesiredRotation = false;
        Move->bOrientRotationToMovement = true;  // face acceleration/velocity
        Move->RotationRate = FRotator(0.f, 540.f, 0.f);

        if (CameraBoom) CameraBoom->bUsePawnControlRotation = true; // keep orbiting with mouse
    }
}

//handles multiplayer
void AProtagonistCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    AddDefaultIMC_Local();
}

void AProtagonistCharacter::UnPossessed()
{
    RemoveDefaultIMC_Local();
    Super::UnPossessed();
}

void AProtagonistCharacter::AddDefaultIMC_Local()
{
    if (!IsLocallyControlled() || !DefaultIMC) return;

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsys =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
            {
                Subsys->AddMappingContext(DefaultIMC, IMCPriority);
            }
        }
    }
}

void AProtagonistCharacter::RemoveDefaultIMC_Local()
{
    if (!IsLocallyControlled() || !DefaultIMC) return;

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsys =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
            {
                Subsys->RemoveMappingContext(DefaultIMC);
            }
        }
    }
}
