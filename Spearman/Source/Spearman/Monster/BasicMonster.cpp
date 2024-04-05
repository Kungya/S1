// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonster.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/WidgetComponent.h"
#include "Spearman/HUD/HpBarWidget.h"

ABasicMonster::ABasicMonster()
{
	PrimaryActorTick.bCanEverTick = true;

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
}

void ABasicMonster::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ // server only
	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UE_LOG(LogTemp, Warning, TEXT("Hp : %f"), Hp);
	if (FMath::IsNearlyZero(Hp))
	{ // Death
		// TODO : GameMode, SpearmanCharacter->GameState?
		
		//Destroy();
		SetLifeSpan(2.f);
	}
	else
	{ // HitReact
		// TODO : PlayHitReactMontage
	}
}

void ABasicMonster::ShowHpBar()
{
	HpBar->SetVisibility(true);
	GetWorldTimerManager().ClearTimer(HpBarTimer);
	GetWorldTimerManager().SetTimer(HpBarTimer, this, &ABasicMonster::HideHpBar, HpBarDisplayTime);
}

void ABasicMonster::HideHpBar()
{
	HpBar->SetVisibility(false);
}

void ABasicMonster::Die()
{
	HideHpBar();

	// TODO : many thing,
}

void ABasicMonster::OnRep_Hp()
{ // client only
	HpBarWidget->SetHpBar(GetHpRatio());
	if (FMath::IsNearlyZero(Hp))
	{
		Die();
	}
}