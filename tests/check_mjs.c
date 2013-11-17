#include <check.h>
#include <stdlib.h>

#include "songdata/songdata.h"

static songdata * empty_list(){
	songdata * list = malloc( sizeof ( songdata ) );
    memset(list, 0, sizeof(songdata));
    return list;
}

static songdata_song * empty_song(char * filename){
    songdata_song * song = new_songdata_song();
    song->filename = strdup(filename);
    song->fullpath = strdup(filename);
    return song;
}

START_TEST (test_song_add_sorted_one)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");

    //assumptions
    ck_assert_int_eq(list->length, 0);
    ck_assert_msg(list->head == list->tail, "head and tail not the same");

    songdata_add_ordered(list, song);

    //Results
    ck_assert_int_eq(list->length, 1);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head == list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the added song");

}
END_TEST;

START_TEST (test_song_add_sorted_simple_two)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");

    songdata_add_ordered(list, song);
    songdata_add_ordered(list, song2);

    //Expected 2 elements A(head)->B(tail)

    //Results
    ck_assert_int_eq(list->length, 2);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song2, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == NULL, "next of the second is NULL");
}
END_TEST;


START_TEST (test_song_add_sorted_simple_three)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song);
    songdata_add_ordered(list, song2);
    songdata_add_ordered(list, song3);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;

START_TEST (test_song_add_sorted_simple_two_reverse)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");

	//First add B than A
    songdata_add_ordered(list, song2);
    songdata_add_ordered(list, song);

    //Expected 2 elements A(head)->B(tail)

    //Results
    ck_assert_int_eq(list->length, 2);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first song");
    ck_assert_msg(list->tail == song2, "tail is the last song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == NULL, "next of the second is NULL");
}
END_TEST;


START_TEST (test_song_add_sorted_simple_three_suffle)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song);
    songdata_add_ordered(list, song3);
    songdata_add_ordered(list, song2);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;


START_TEST (test_song_add_sorted_simple_three_suffle_2)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song3);
    songdata_add_ordered(list, song2);
    songdata_add_ordered(list, song);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;

START_TEST (test_song_add_sorted_simple_three_suffle_3)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song3);
    songdata_add_ordered(list, song);
    songdata_add_ordered(list, song2);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;


START_TEST (test_song_add_sorted_simple_three_suffle_4)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song2);
    songdata_add_ordered(list, song3);
    songdata_add_ordered(list, song);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;

START_TEST (test_song_add_sorted_simple_three_suffle_5)
{

	songdata * list = empty_list();
	songdata_song * song = empty_song("A");
	songdata_song * song2 = empty_song("B");
	songdata_song * song3 = empty_song("C");

    songdata_add_ordered(list, song2);
    songdata_add_ordered(list, song);
    songdata_add_ordered(list, song3);

    //Expected 3 elements A(head)->B->C(tail)

    //Results
    ck_assert_int_eq(list->length, 3);
    ck_assert_int_eq(list->where, 1);
    ck_assert_msg(list->head != list->tail, "head and tail not the same");
    ck_assert_msg(list->head == list->selected, "head and selected not the same");
    ck_assert_msg(list->head == song, "head is the first added song");
    ck_assert_msg(list->tail == song3, "tail is the last added song");
    ck_assert_msg(song->prev == NULL, "no prev for the first");
    ck_assert_msg(song->next == song2, "next of the first is the second");
    ck_assert_msg(song2->prev == song, "prev of the second is the first");
    ck_assert_msg(song2->next == song3, "next of the second is song3");
    ck_assert_msg(song3->prev == song2, "prev of the song3 is song2");
    ck_assert_msg(song3->next == NULL, "next of the song3 is NULL");
}
END_TEST;

Suite * mjs_suite (void)
{
  Suite *s = suite_create ("MJS");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_test (tc_core, test_song_add_sorted_one);
  tcase_add_test (tc_core, test_song_add_sorted_simple_two);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three);
  tcase_add_test (tc_core, test_song_add_sorted_simple_two_reverse);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three_suffle);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three_suffle_2);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three_suffle_3);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three_suffle_4);
  tcase_add_test (tc_core, test_song_add_sorted_simple_three_suffle_5);
  suite_add_tcase (s, tc_core);

  return s;
}
 int
 main (void)
 {
  int number_failed;
  Suite *s = mjs_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
