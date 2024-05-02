// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ItemDragWidget.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"


US1ItemDragWidget::US1ItemDragWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void US1ItemDragWidget::Init(const FVector2D InWidgetSize, UTexture2D* InItemIcon, int32 InItemCount)
{
	// �������� 1ĭ�� �Ѿ�ٸ� �װ� �޾ƿ� ���
	Root_SizeBox->SetWidthOverride(InWidgetSize.X);
	Root_SizeBox->SetHeightOverride(InWidgetSize.Y);

	Icon_Image->SetBrushFromTexture(InItemIcon);
	Count_Text->SetText((InItemCount >= 2) ? FText::AsNumber(InItemCount) : FText::GetEmpty());
}
