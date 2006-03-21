/*
 * Copyright (C) 2005 National Association of REALTORS(R)
 *
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished
 * to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software and that
 * both the above copyright notice(s) and this permission notice
 * appear in supporting documentation.
 */
#include "TranslationWorkers.h"
#include "DateTimeFormatException.h"
#include "str_stream.h"
#include "utils.h"

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace odbcrets;
using std::string;
using boost::lexical_cast;
using boost::numeric_cast;
namespace b = boost;

TranslationWorker::~TranslationWorker()
{
}

void TranslationWorker::setResultSize(SQLLEN *resultSize, SQLLEN value)
{
    if (resultSize)
    {
        *resultSize = value;
    }
}

SQLSMALLINT BitTranslationWorker::getOdbcType() { return SQL_BIT; }

string BitTranslationWorker::getOdbcTypeName() { return "BOOLEAN"; }

int BitTranslationWorker::getOdbcTypeLength() { return sizeof(char); }

void BitTranslationWorker::translate(string data, SQLPOINTER target,
                                     SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    char* result = (char*) target;
    if (data.compare("0") == 0)
    {
        *result = 0;
    }
    else
    {
        *result = 1;
    }

    setResultSize(resultSize, sizeof(char));
}

SQLSMALLINT DateTranslationWorker::getOdbcType() { return SQL_TYPE_DATE; }

string DateTranslationWorker::getOdbcTypeName() { return "DATE"; }

int DateTranslationWorker::getOdbcTypeLength() { return SQL_DATE_LEN; }

void DateTranslationWorker::translate(string data, SQLPOINTER target,
                                      SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    int dataSize = data.size();
    if ((dataSize >= 8) && (dataSize <= 12) && (data.at(2) == ':'))
    {
        // We have to be a time, we don't do that here
        setResultSize(resultSize, SQL_NULL_DATA);
        throw DateTimeFormatException("Value not a date: " + data);
    }

    DATE_STRUCT* date = (DATE_STRUCT*) target;

    try
    {
        date->year = lexical_cast<SQLSMALLINT>(data.substr(0,4));
        date->month = lexical_cast<SQLSMALLINT>(data.substr(5,2));
        date->day = lexical_cast<SQLSMALLINT>(data.substr(8,2));
    }
    catch(b::bad_lexical_cast&)
    {
        throw DateTimeFormatException(
            str_stream() << "bad_lexical_cast: could not convert \""
            << data << "\" to Date");
    }

    setResultSize(resultSize, SQL_DATE_LEN);
}

SQLSMALLINT TimestampTranslationWorker::getOdbcType()
{
    return SQL_TYPE_TIMESTAMP;
}

string TimestampTranslationWorker::getOdbcTypeName() { return "DATE_TIME"; }

int TimestampTranslationWorker::getOdbcTypeLength()
{
    return SQL_TIMESTAMP_LEN;
}

void TimestampTranslationWorker::translate(string data, SQLPOINTER target,
                                           SQLLEN targetLen,
                                           SQLLEN *resultSize)
{
    // The following definitions are from the Rets spec.  We should
    // be able to handle most of the following:
    //
    // Date -- A date, in YYYY-MM-DD format. 10
    // DateTime -- A timestamp, in YYYY-MM-DDThh:mm:ss[.sss] format. 19:23
    // Time -- A time, stored in hh:mm:ss[.sss] format. 8:12

    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    int dataSize = data.size();
    if ((dataSize >= 8) && (dataSize <= 12) && (data.at(2) == ':'))
    {
        // We have to be a time, we don't do that here
        setResultSize(resultSize, SQL_NULL_DATA);
        throw DateTimeFormatException("Value not a timestamp: " + data);
    }

    // we're a time, return sqlerror;
    TIMESTAMP_STRUCT* tm = (TIMESTAMP_STRUCT*) target;

    try
    {
        tm->year = lexical_cast<SQLSMALLINT>(data.substr(0,4));
        tm->month = lexical_cast<SQLSMALLINT>(data.substr(5,2));
        tm->day = lexical_cast<SQLSMALLINT>(data.substr(8,2));
        if (dataSize == 10)
        {
            // we're a Date
            tm->hour = 0;
            tm->minute = 0;
            tm->second = 0;
            tm->fraction = 0;
        }
        else
        {
            // Offset here due to ambigueity in spec.  Is the T required?
            // Is there a space after that questionably required T?
        
            // o is for Offset!  M is for monkey
            int o = 0;
            if (data.at(10) == ' ' || data.at(10) == 'T')
            {
                o++;
            }
            if (data.at(11) == 'T')
            {
                o++;
            }
        
            // we're a date time / timestamp
            tm->hour = lexical_cast<SQLSMALLINT>(data.substr(10 + o, 2));
            tm->minute = lexical_cast<SQLSMALLINT>(data.substr(13 + o, 2));
            tm->second = lexical_cast<SQLSMALLINT>(data.substr(16 + o, 2));
            // TODO: support fractions of a second, but really, are you going
            // to run into that in real estate data?
            tm->fraction=0;
        }
    }
    catch(b::bad_lexical_cast&)
    {
        throw DateTimeFormatException(
            str_stream() << "bad_lexical_cast: could not convert \""
            << data << "\" to Timestamp");
    }
        
    setResultSize(resultSize, SQL_TIMESTAMP_LEN);
}

SQLSMALLINT TimeTranslationWorker::getOdbcType() { return SQL_TYPE_TIME; }

string TimeTranslationWorker::getOdbcTypeName() { return "TIME"; }

int TimeTranslationWorker::getOdbcTypeLength() { return SQL_TIME_LEN; }

void TimeTranslationWorker::translate(string data, SQLPOINTER target,
                                      SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    int dataSize = data.size();
    if (!((dataSize >= 8) && (dataSize <= 12) && (data.at(2) == ':')))
    {
        // We can't be a date if we don't fit in this range
        setResultSize(resultSize, SQL_NULL_DATA);
        throw DateTimeFormatException("Value not a time: " + data);
    }

    TIME_STRUCT* tm = (TIME_STRUCT*) target;
    try
    {
        tm->hour = lexical_cast<SQLSMALLINT>(data.substr(0, 2));
        tm->minute = lexical_cast<SQLSMALLINT>(data.substr(3, 2));
        tm->second = lexical_cast<SQLSMALLINT>(data.substr(6, 2));
    }
    catch(b::bad_lexical_cast&)
    {
        throw DateTimeFormatException(
            str_stream() << "bad_lexical_cast: could not convert \""
            << data << "\" to Time");
    }
    

    setResultSize(resultSize, SQL_TIME_LEN);
}

SQLSMALLINT TinyTranslationWorker::getOdbcType() { return SQL_TINYINT; }

string TinyTranslationWorker::getOdbcTypeName() { return "TINY"; }

int TinyTranslationWorker::getOdbcTypeLength() { return sizeof(char); }

void TinyTranslationWorker::translate(string data, SQLPOINTER target,
                                      SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    char* output = (char*) target;
    
    *output = numeric_cast<char>(lexical_cast<int>(data));
    setResultSize(resultSize, sizeof(char));
}

SQLSMALLINT SmallIntTranslationWorker::getOdbcType()
{
    return SQL_SMALLINT;
}

string SmallIntTranslationWorker::getOdbcTypeName() { return "SMALL"; }

int SmallIntTranslationWorker::getOdbcTypeLength()
{
    return sizeof(SQLSMALLINT);
}

void SmallIntTranslationWorker::translate(string data, SQLPOINTER target,
                                          SQLLEN targetLen,
                                          SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    SQLSMALLINT* result = (SQLSMALLINT*) target;
    *result = lexical_cast<SQLSMALLINT>(data);
    setResultSize(resultSize, sizeof(SQLSMALLINT));
}

SQLSMALLINT IntTranslationWorker::getOdbcType() { return SQL_INTEGER; }

string IntTranslationWorker::getOdbcTypeName() { return "INT"; }

int IntTranslationWorker::getOdbcTypeLength() { return sizeof(SQLINTEGER); }

void IntTranslationWorker::translate(string data, SQLPOINTER target,
                                     SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    SQLINTEGER* result = (SQLINTEGER*) target;
    *result = lexical_cast<SQLINTEGER>(data);
    setResultSize(resultSize, sizeof(SQLINTEGER));
}

SQLSMALLINT BigIntTranslationWorker::getOdbcType() { return SQL_BIGINT; }

string BigIntTranslationWorker::getOdbcTypeName() { return "LONG"; }

int BigIntTranslationWorker::getOdbcTypeLength() { return sizeof(SQLBIGINT); }

void BigIntTranslationWorker::translate(string data, SQLPOINTER target,
                                        SQLLEN targetLen, SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    setResultSize(resultSize, SQL_NULL_DATA);

    // This doesn't work for BIGINT
    //     std::istringstream in(data);
    //     SQLBIGINT* result = (SQLBIGINT*) target;
    //     in >> *result;

    // Nor does this
    //    SQLBIGINT v = lexical_cast<SQLBIGINT>(data);
}

SQLSMALLINT DecimalTranslationWorker::getOdbcType()
{
    return SQL_DECIMAL;
}

string DecimalTranslationWorker::getOdbcTypeName() { return "DECIMAL"; }

int DecimalTranslationWorker::getOdbcTypeLength()
{
    return sizeof(SQLDECIMAL);
}

void DecimalTranslationWorker::translate(string data, SQLPOINTER target,
                                         SQLLEN targetLen,
                                         SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    SQLDECIMAL* result = (SQLDECIMAL*) target;
    *result = lexical_cast<SQLDECIMAL>(data);
    setResultSize(resultSize, sizeof(SQLDECIMAL));
}

SQLSMALLINT CharacterTranslationWorker::getOdbcType() { return SQL_VARCHAR; }

string CharacterTranslationWorker::getOdbcTypeName() { return "CHARACTER"; }

int CharacterTranslationWorker::getOdbcTypeLength() { return -1; }

void CharacterTranslationWorker::translate(
    string data, SQLPOINTER target, SQLLEN targetLen, SQLLEN *resultSize)
{
    size_t size = copyString(data, (char *) target, targetLen);
    setResultSize(resultSize, size);
}

SQLSMALLINT DoubleTranslationWorker::getOdbcType()
{
    return SQL_DOUBLE;
}

string DoubleTranslationWorker::getOdbcTypeName() { return "DOUBLE"; }

int DoubleTranslationWorker::getOdbcTypeLength() { return sizeof(SQLDOUBLE); }

void DoubleTranslationWorker::translate(string data, SQLPOINTER target,
                                        SQLLEN targetLen,
                                        SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    SQLDOUBLE* result = (SQLDOUBLE*) target;
    *result = lexical_cast<SQLDOUBLE>(data);
    setResultSize(resultSize, sizeof(SQLDOUBLE));
}

SQLSMALLINT BinaryTranslationWorker::getOdbcType()
{
    return SQL_LONGVARBINARY;
}

std::string BinaryTranslationWorker::getOdbcTypeName()
{
    return "LONG VARBINARY";
}

int BinaryTranslationWorker::getOdbcTypeLength() { return -1; }

void BinaryTranslationWorker::translate(string data, SQLPOINTER target,
                                        SQLLEN targetLen,
                                        SQLLEN *resultSize)
{
    if (data.empty() || b::trim_copy(data).empty())
    {
        setResultSize(resultSize, SQL_NULL_DATA);
        return;
    }

    // What to do with a binary?
    if ((target == NULL) || (targetLen == 0))
    {
        setResultSize(resultSize, 0);
        return;
    }

    SQLLEN size = data.copy((char*) target, targetLen);
    setResultSize(resultSize, size);
}
