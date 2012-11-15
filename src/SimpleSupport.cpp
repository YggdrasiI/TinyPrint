#include <QDataStream>
#include "SimpleSupport.h"

void SimpleSupport::streamInSupport(QDataStream* pIn){
  quint32 temp;
  *pIn >> mPoint >> temp >> mSize >> mStart >> mEnd;
  mType = (SupportType)temp;
}


