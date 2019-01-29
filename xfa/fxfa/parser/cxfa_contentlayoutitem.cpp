// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_contentlayoutitem.h"

#include "fxjs/xfa/cjx_object.h"
#include "xfa/fxfa/parser/cxfa_margin.h"
#include "xfa/fxfa/parser/cxfa_node.h"

CXFA_ContentLayoutItem::CXFA_ContentLayoutItem(CXFA_Node* pNode)
    : CXFA_LayoutItem(pNode, kContentItem) {}

CXFA_ContentLayoutItem::~CXFA_ContentLayoutItem() {
  RemoveSelf();
  if (m_pFormNode->JSObject()->GetLayoutItem() == this)
    m_pFormNode->JSObject()->SetLayoutItem(nullptr);
}

CXFA_FFWidget* CXFA_ContentLayoutItem::AsFFWidget() {
  return nullptr;
}

CXFA_ContentLayoutItem* CXFA_ContentLayoutItem::GetFirst() {
  CXFA_ContentLayoutItem* pCurNode = this;
  while (auto* pPrev = pCurNode->GetPrev())
    pCurNode = pPrev;

  return pCurNode;
}

CXFA_ContentLayoutItem* CXFA_ContentLayoutItem::GetLast() {
  CXFA_ContentLayoutItem* pCurNode = this;
  while (auto* pNext = pCurNode->GetNext())
    pCurNode = pNext;

  return pCurNode;
}

void CXFA_ContentLayoutItem::InsertAfter(CXFA_ContentLayoutItem* pItem) {
  pItem->RemoveSelf();
  pItem->m_pNext = m_pNext;
  pItem->m_pPrev = this;
  m_pNext = pItem;
  if (pItem->m_pNext)
    pItem->m_pNext->m_pPrev = pItem;
}

void CXFA_ContentLayoutItem::RemoveSelf() {
  if (m_pNext)
    m_pNext->m_pPrev = m_pPrev;
  if (m_pPrev)
    m_pPrev->m_pNext = m_pNext;
}

CFX_RectF CXFA_ContentLayoutItem::GetRect(bool bRelative) const {
  CFX_PointF sPos = m_sPos;
  CFX_SizeF sSize = m_sSize;
  if (bRelative)
    return CFX_RectF(sPos, sSize);

  for (CXFA_LayoutItem* pLayoutItem = m_pParent; pLayoutItem;
       pLayoutItem = pLayoutItem->m_pParent) {
    if (CXFA_ContentLayoutItem* pContent = pLayoutItem->AsContentLayoutItem()) {
      sPos += pContent->m_sPos;
      CXFA_Margin* pMarginNode =
          pContent->m_pFormNode->GetFirstChildByClass<CXFA_Margin>(
              XFA_Element::Margin);
      if (pMarginNode) {
        sPos += CFX_PointF(pMarginNode->JSObject()
                               ->GetMeasure(XFA_Attribute::LeftInset)
                               .ToUnit(XFA_Unit::Pt),
                           pMarginNode->JSObject()
                               ->GetMeasure(XFA_Attribute::TopInset)
                               .ToUnit(XFA_Unit::Pt));
      }
      continue;
    }

    if (pLayoutItem->GetFormNode()->GetElementType() ==
        XFA_Element::ContentArea) {
      sPos += CFX_PointF(pLayoutItem->GetFormNode()
                             ->JSObject()
                             ->GetMeasure(XFA_Attribute::X)
                             .ToUnit(XFA_Unit::Pt),
                         pLayoutItem->GetFormNode()
                             ->JSObject()
                             ->GetMeasure(XFA_Attribute::Y)
                             .ToUnit(XFA_Unit::Pt));
      break;
    }
    if (pLayoutItem->GetFormNode()->GetElementType() == XFA_Element::PageArea)
      break;
  }
  return CFX_RectF(sPos, sSize);
}

int32_t CXFA_ContentLayoutItem::GetIndex() const {
  int32_t iIndex = 0;
  const CXFA_ContentLayoutItem* pCurNode = this;
  while (auto* pPrev = pCurNode->GetPrev()) {
    pCurNode = pPrev;
    ++iIndex;
  }
  return iIndex;
}

int32_t CXFA_ContentLayoutItem::GetCount() const {
  int32_t iCount = GetIndex() + 1;
  const CXFA_ContentLayoutItem* pCurNode = this;
  while (auto* pNext = pCurNode->GetNext()) {
    pCurNode = pNext;
    iCount++;
  }
  return iCount;
}
