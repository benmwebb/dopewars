/************************************************************************
 * test-tstring.c Unit tests for translated format strings              *
 * Copyright (C)  2026  Ben Webb                                        *
 *                Email: benwebb@users.sf.net                           *
 *                WWW: https://dopewars.sourceforge.io/                 *
 *                                                                      *
 * This program is free software; you can redistribute it and/or        *
 * modify it under the terms of the GNU General Public License          *
 * as published by the Free Software Foundation; either version 2       *
 * of the License, or (at your option) any later version.               *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,               *
 *                   MA  02111-1307, USA.                               *
 ************************************************************************/

#include <string.h>

#include "common.h"
#include "tstring.h"

/* Checks that dpg_strdup_printf gives "expected" */
#define assert_tfmt(expected, ...) G_STMT_START {        \
    gchar *_str = dpg_strdup_printf(__VA_ARGS__);       \
    g_assert_cmpstr(_str, ==, expected);                \
    g_free(_str);                                       \
  } G_STMT_END

static void test_basic(void)
{
  gchar *price, *expected;

  assert_tfmt("no arguments", "no arguments");
  assert_tfmt("7 dogs x", "%d %s %c", 7, "dogs", 'x');
  assert_tfmt("[   42|ab   ]", "[%5d|%-5s]", 42, "ab");
  assert_tfmt("abc", "%.3s", "abcdef");

  price = FormatPrice(1234);
  expected = g_strdup_printf("cost %s", price);
  assert_tfmt(expected, "cost %P", (price_t)1234);
  g_free(expected);
  g_free(price);
}

static void test_translated(void)
{
  assert_tfmt("Buy cocaine", "Buy %tde", "cocaine");
  assert_tfmt("Buy Cocaine", "Buy %Tde", "cocaine");
  /* Strings can carry a translation for a given code */
  assert_tfmt("a bitch", "a %tde", "bitch_fr_pute");
  assert_tfmt("une pute", "une %tfr", "bitch_fr_pute");
}

static void test_positional(void)
{
  assert_tfmt("two one", "%2$s %1$s", "one", "two");
  assert_tfmt("5 apples, 5 times", "%1$d %2$s, %1$d times", 5, "apples");
}

/*
 * %% and %/comment/ don't take an argument, so they mustn't shift the
 * numbering of the arguments, with or without %n$.
 */
static void test_percent_and_comment(void)
{
  assert_tfmt("100% sure", "%d%% %s", 100, "sure");
  assert_tfmt("Cops bust cocaine", "%/Bust message/Cops bust %tde",
              "cocaine");
  assert_tfmt("cocaine is 50% off", "%/Comment/%2$s is %1$d%% off",
              50, "cocaine");
  assert_tfmt("50% off cocaine", "%1$d%% off %/Comment/%2$tde",
              50, "cocaine");
}

/* Invalid format strings are returned as-is, with a warning, rather than
 * aborting or reading arguments of the wrong type */
static void check_invalid(gchar *format)
{
  gchar *str;

  g_test_expect_message(NULL, G_LOG_LEVEL_WARNING,
                        "Invalid format string*");
  str = dpg_strdup_printf(format, "one", "two");
  g_test_assert_expected_messages();
  g_assert_cmpstr(str, ==, format);
  g_free(str);
}

static void test_invalid(void)
{
  check_invalid("unknown %q conversion");
  check_invalid("trailing %");
  check_invalid("%2$s only");                    /* Argument 1 unused */
  check_invalid("%1$s %1$d");                    /* Type mismatch */
  check_invalid("%1$tde %1$Tde");
  check_invalid("%1001$s");                      /* Too many arguments */
  check_invalid("%2147483648$s");                /* Used to overflow */
  check_invalid("%99999999999999999999$s");
  check_invalid("%99999999999s");                /* Width too big */
  check_invalid("%.99999999999s");               /* Precision too big */
}

static void test_is_valid(void)
{
  g_assert_true(IsValidTFmt("no arguments", "s"));
  g_assert_true(IsValidTFmt("%s", "s"));
  g_assert_true(IsValidTFmt("Addicts are buying %tde!", "s"));
  g_assert_true(IsValidTFmt("%Tde!", "s"));
  g_assert_true(IsValidTFmt("100%% %/comment/%-20s", "s"));
  g_assert_true(IsValidTFmt("%1$s and %1$s", "s"));
  g_assert_true(IsValidTFmt("%d %s %c %P", "dscP"));
  g_assert_true(IsValidTFmt("%2$s %1$d", "ds"));
  g_assert_true(IsValidTFmt(DefaultDrugs.ExpensiveStr1, "s"));
  g_assert_true(IsValidTFmt(DefaultDrugs.ExpensiveStr2, "s"));

  g_assert_false(IsValidTFmt("%d", "s"));        /* Wrong type */
  g_assert_false(IsValidTFmt("%s %s", "s"));     /* Too many arguments */
  g_assert_false(IsValidTFmt("%2$s", "ss"));     /* Argument 1 unused */
  g_assert_false(IsValidTFmt("%x %p %n", "sss"));
  g_assert_false(IsValidTFmt("%s %", "s"));
  g_assert_false(IsValidTFmt("%2147483648$s", "s"));
  g_assert_false(IsValidTFmt("%1001s", "s"));
}

int main(int argc, char *argv[])
{
  dptest_init(&argc, &argv);

  g_test_add_func("/tstring/basic", test_basic);
  g_test_add_func("/tstring/translated", test_translated);
  g_test_add_func("/tstring/positional", test_positional);
  g_test_add_func("/tstring/percent-and-comment", test_percent_and_comment);
  g_test_add_func("/tstring/invalid", test_invalid);
  g_test_add_func("/tstring/is-valid", test_is_valid);

  return g_test_run();
}
