// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonster.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Spearman/HUD/HitDamageWidget.h"
#include "Spearman/HUD/HpBarWidget.h"

ABasicMonster::ABasicMonster()
{
	PrimaryActorTick.bCanEverTick = true;

	HitDamage = CreateDefaultSubobject<UWidgetComponent>(TEXT("HitDamageWidget"));
	HitDamage->SetupAttachment(GetMesh());

	HpBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HpBarWidget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetWidgetSpace(EWidgetSpace::Screen);
	HpBar->SetDrawSize(FVector2D(125.f, 20.f));
}

void ABasicMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABasicMonster, Hp);
}

void ABasicMonster::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	HitDamage->InitWidget();
	HitDamageWidget = Cast<UHitDamageWidget>(HitDamage->GetUserWidgetObject());

	HpBar->InitWidget();
	HpBarWidget = Cast<UHpBarWidget>(HpBar->GetUserWidgetObject());
}

void ABasicMonster::BeginPlay()
{
	Super::BeginPlay();

	HpBarWidget->SetHpBar(GetHpRatio());
	HpBar->SetVisibility(false);


	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABasicMonster::OnAttacked);
	}
}

void ABasicMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	UpdateHitDamages();
}

void ABasicMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABasicMonster::WeaponHit_Implementation(FHitResult HitResult)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitResult.Location, FRotator(0.f), true);
	}

	ShowHpBar();
	PlayHitMontage(FName("HitReact"));
}

void ABasicMonster::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ // server only
	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UE_LOG(LogTemp, Warning, TEXT("Hp : %f"), Hp);
	if (FMath::IsNearlyZero(Hp))
	{ // Death
		// TODO : GameMode, SpearmanCharacter->GameState?
		
		// TODO : Warning use Widget After Destory kinda Timer
		//Destroy();
		SetLifeSpan(2.f);
	}
	else
	{ // HitReact
		// TODO : PlayHitReactMontage
	}
}

void ABasicMonster::ShowHitDamage(int32 Damage, FVector HitLocation, bool bHeadShot)
{
	/*FVector2D HitDamagePosition;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController)
	{
		UGameplayStatics::ProjectWorldToScreen(PlayerController, HitLocation, HitDamagePosition);
	}*/
	/*const float RandX = FMath::RandRange(HitDamagePosition.X - 100.f, HitDamagePosition.X + 100.f);
	const float RandY = FMath::RandRange(HitDamagePosition.Y - 100.f, HitDamagePosition.Y + 100.f);
	HitDamagePosition = FVector2D(RandX, RandY);*/

	HitDamageWidget->SetHitDamageText(Damage);
	HitDamageWidget->AddToViewport();
	//HitDamageWidget->SetPositionInViewport(HitDamagePosition);
	HitDamageWidget->PlayHitDamageAnimation(bHeadShot);
	StoreHitDamage(HitDamageWidget, HitLocation);
}

void ABasicMonster::StoreHitDamage(UHitDamageWidget* HitDamageToStore, FVector Location)
{
	HitDamages.Add(HitDamageToStore, Location);


	FTimerHandle HitDamageTimer;
	FTimerDelegate HitDamageDelegate;
	HitDamageDelegate.BindUFunction(this, FName("DestroyHitDamage"), HitDamageToStore);
	GetWorld()->GetTimerManager().SetTimer(HitDamageTimer, HitDamageDelegate, HitDamageDestroyTime, false);
}

void ABasicMonster::DestroyHitDamage(UHitDamageWidget* HitDamageToDestroy)
{
	HitDamages.Remove(HitDamageToDestroy);
	HitDamageToDestroy->RemoveFromParent();
}

void ABasicMonster::UpdateHitDamages()
{
	for (const auto& HitDamagePair : HitDamages)
	{
		UHitDamageWidget* HDWidget = HitDamagePair.Key;
		const FVector Location = HitDamagePair.Value;
		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HDWidget->SetPositionInViewport(ScreenPosition);
	}
}

void ABasicMonster::ShowHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(true);
	}
	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ABasicMonster::HideHpBar, HpBarDisplayTime);
}

void ABasicMonster::HideHpBar()
{
	if (HpBar)
	{
		HpBar->SetVisibility(false);
	}
}

void ABasicMonster::Die()
{
	HideHpBar();

	// TODO : many thing,
}

void ABasicMonster::PlayHitMontage(FName Section, float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HitMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, HitMontage);
	}
}

void ABasicMonster::OnRep_Hp()
{ // client only
	HpBarWidget->SetHpBar(GetHpRatio());
	if (FMath::IsNearlyZero(Hp))
	{
		Die();
	}
}