
#pragma once
#include <stdio.h>
#include <QDebug>
#define LogDebug(format, ...)    qDebug("[%s::%s::%d]"format,__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define EnterLine  qDebug() << "Enter:" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__
#define ExitLine  qDebug() << "Exit:" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__

