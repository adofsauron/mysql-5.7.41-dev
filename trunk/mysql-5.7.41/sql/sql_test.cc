/* Copyright (c) 2000, 2022, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

/* Write some debug info */

#include "sql_test.h"
#include "sql_base.h"  // table_def_cache, table_cache_count, unused_tables
#include "sql_show.h"  // calc_sum_of_all_status
#include "sql_select.h"
#include "opt_trace.h"
#include "keycaches.h"
#include "sql_optimizer.h"  // JOIN
#include "opt_explain.h"    // join_type_str
#include <hash.h>
#ifndef EMBEDDED_LIBRARY
#include "events.h"
#endif
#include "table_cache.h"         // table_cache_manager
#include "mysqld_thd_manager.h"  // Global_THD_manager
#include "prealloced_array.h"

#include <algorithm>
#include <functional>

#if defined(HAVE_MALLOC_INFO) && defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

const char *lock_descriptions[TL_WRITE_ONLY + 1] = {
    /* TL_UNLOCK                  */ "No lock",
    /* TL_READ_DEFAULT            */ NULL,
    /* TL_READ                    */ "Low priority read lock",
    /* TL_READ_WITH_SHARED_LOCKS  */ "Shared read lock",
    /* TL_READ_HIGH_PRIORITY      */ "High priority read lock",
    /* TL_READ_NO_INSERT          */ "Read lock without concurrent inserts",
    /* TL_WRITE_ALLOW_WRITE       */ "Write lock that allows other writers",
    /* TL_WRITE_CONCURRENT_DEFAULT*/ NULL,
    /* TL_WRITE_CONCURRENT_INSERT */ "Concurrent insert lock",
    /* TL_WRITE_DEFAULT           */ NULL,
    /* TL_WRITE_LOW_PRIORITY      */ "Low priority write lock",
    /* TL_WRITE                   */ "High priority write lock",
    /* TL_WRITE_ONLY              */ "Highest priority write lock"};

#ifndef NDEBUG

void print_where(Item *cond, const char *info, enum_query_type query_type)
{
  char buff[256];
  String str(buff, sizeof(buff), system_charset_info);
  str.length(0);
  if (cond)
    cond->print(&str, query_type);
  str.append('\0');

  DBUG_LOCK_FILE;
  (void)fprintf(DBUG_FILE, "\nWHERE:(%s) %p ", info, cond);
  (void)fputs(str.ptr(), DBUG_FILE);
  (void)fputc('\n', DBUG_FILE);
  DBUG_UNLOCK_FILE;
}
/* This is for debugging purposes */

static void print_cached_tables(void)
{
  /* purecov: begin tested */
  table_cache_manager.lock_all_and_tdc();

  table_cache_manager.print_tables();

  printf("\nCurrent refresh version: %ld\n", refresh_version);
  if (my_hash_check(&table_def_cache))
    printf("Error: Table definition hash table is corrupted\n");
  fflush(stdout);
  table_cache_manager.unlock_all_and_tdc();
  /* purecov: end */
  return;
}

void TEST_join(JOIN *join)
{
  uint i, ref;
  DBUG_ENTER("TEST_join");
  assert(!join->join_tab);
  /*
    Assemble results of all the calls to full_name() first,
    in order not to garble the tabular output below.
  */
  String ref_key_parts[MAX_TABLES];
  for (i = 0; i < join->tables; i++)
  {
    JOIN_TAB *tab = join->best_ref[i];
    for (ref = 0; ref < tab->ref().key_parts; ref++)
    {
      ref_key_parts[i].append(tab->ref().items[ref]->full_name());
      ref_key_parts[i].append("  ");
    }
  }

  DBUG_LOCK_FILE;
  (void)fputs("\nInfo about JOIN\n", DBUG_FILE);
  for (i = 0; i < join->tables; i++)
  {
    JOIN_TAB *tab = join->best_ref[i];
    TABLE *form = tab->table();
    if (!form)
      continue;
    char key_map_buff[128];
    fprintf(DBUG_FILE, "%-16.16s  type: %-7s  q_keys: %s  refs: %d  key: %d  len: %d\n", form->alias,
            join_type_str[tab->type()], tab->keys().print(key_map_buff), tab->ref().key_parts, tab->ref().key,
            tab->ref().key_length);
    if (tab->quick())
    {
      char buf[MAX_KEY / 8 + 1];
      if (tab->use_quick == QS_DYNAMIC_RANGE)
        fprintf(DBUG_FILE, "                  quick select checked for each record (keys: %s)\n",
                form->quick_keys.print(buf));
      else
      {
        fprintf(DBUG_FILE, "                  quick select used:\n");
        tab->quick()->dbug_dump(18, FALSE);
      }
    }
    if (tab->ref().key_parts)
    {
      fprintf(DBUG_FILE, "                  refs:  %s\n", ref_key_parts[i].ptr());
    }
  }
  DBUG_UNLOCK_FILE;
  DBUG_VOID_RETURN;
}

#endif /* !NDEBUG */

void print_keyuse_array(Opt_trace_context *trace, const Key_use_array *keyuse_array)
{
#if !defined(NDEBUG) || defined(OPTIMIZER_TRACE)
  if (unlikely(!trace->is_started()))
    return;
  Opt_trace_object wrapper(trace);
  Opt_trace_array trace_key_uses(trace, "ref_optimizer_key_uses");
  DBUG_PRINT("opt", ("Key_use array (%zu elements)", keyuse_array->size()));
  for (uint i = 0; i < keyuse_array->size(); i++)
  {
    const Key_use &keyuse = keyuse_array->at(i);
    // those are too obscure for opt trace
    DBUG_PRINT("opt", ("Key_use: optimize= %d used_tables=0x%llx "
                       "ref_table_rows= %lu keypart_map= %0lx",
                       keyuse.optimize, keyuse.used_tables, (ulong)keyuse.ref_table_rows, keyuse.keypart_map));
    Opt_trace_object(trace)
        .add_utf8_table(keyuse.table_ref)
        .add_utf8("field",
                  (keyuse.keypart == FT_KEYPART)
                      ? "<fulltext>"
                      : keyuse.table_ref->table->key_info[keyuse.key].key_part[keyuse.keypart].field->field_name)
        .add("equals", keyuse.val)
        .add("null_rejecting", keyuse.null_rejecting);
  }
#endif /* !NDEBUG || OPTIMIZER_TRACE */
}

#ifndef NDEBUG
/* purecov: begin inspected */

/**
  Print the current state during query optimization.

  @param join              pointer to the structure providing all context
                           info for the query
  @param idx               length of the partial QEP in 'join->positions'
                           also an index in the array 'join->best_ref'
  @param record_count      estimate for the number of records returned by
                           the best partial plan
  @param read_time         the cost of the best partial plan.
                           If a complete plan is printed (join->best_read is
                           set), this argument is ignored.
  @param current_read_time the accumulated cost of the current partial plan
  @param info              comment string to appear above the printout

  @details
    This function prints to the log file DBUG_FILE the members of 'join' that
    are used during query optimization (join->positions, join->best_positions,
    and join->best_ref) and few other related variables (read_time,
    record_count).
    Useful to trace query optimizer functions.
*/

void print_plan(JOIN *join, uint idx, double record_count, double read_time, double current_read_time, const char *info)
{
  uint i;
  POSITION pos;
  JOIN_TAB *join_table;
  JOIN_TAB **plan_nodes;
  TABLE *table;

  if (info == 0)
    info = "";

  DBUG_LOCK_FILE;
  if (join->best_read == DBL_MAX)
  {
    fprintf(DBUG_FILE, "%s; idx: %u  best: DBL_MAX  atime: %g  itime: %g  count: %g\n", info, idx, current_read_time,
            read_time, record_count);
  }
  else
  {
    fprintf(DBUG_FILE, "%s; idx :%u  best: %g  accumulated: %g  increment: %g  count: %g\n", info, idx, join->best_read,
            current_read_time, read_time, record_count);
  }

  /* Print the tables in JOIN->positions */
  fputs("     POSITIONS: ", DBUG_FILE);
  for (i = 0; i < idx; i++)
  {
    pos = join->positions[i];
    table = pos.table->table();
    if (table)
      fputs(table->alias, DBUG_FILE);
    fputc(' ', DBUG_FILE);
  }
  fputc('\n', DBUG_FILE);

  /*
    Print the tables in JOIN->best_positions only if at least one complete plan
    has been found. An indicator for this is the value of 'join->best_read'.
  */
  if (join->best_read < DBL_MAX)
  {
    fputs("BEST_POSITIONS: ", DBUG_FILE);
    for (i = 0; i < idx; i++)
    {
      pos = join->best_positions[i];
      table = pos.table->table();
      if (table)
        fputs(table->alias, DBUG_FILE);
      fputc(' ', DBUG_FILE);
    }
  }
  fputc('\n', DBUG_FILE);

  /* Print the tables in JOIN->best_ref */
  fputs("      BEST_REF: ", DBUG_FILE);
  for (plan_nodes = join->best_ref; *plan_nodes; plan_nodes++)
  {
    join_table = (*plan_nodes);
    fputs(join_table->table()->s->table_name.str, DBUG_FILE);
    fprintf(DBUG_FILE, "(%lu,%lu,%lu)", (ulong)join_table->found_records, (ulong)join_table->records(),
            (ulong)join_table->read_time);
    fputc(' ', DBUG_FILE);
  }
  fputc('\n', DBUG_FILE);

  DBUG_UNLOCK_FILE;
}

#endif /* !NDEBUG */

C_MODE_START
static int print_key_cache_status(const char *name, KEY_CACHE *key_cache);
C_MODE_END

typedef struct st_debug_lock
{
  my_thread_id thread_id;
  char table_name[FN_REFLEN];
  bool waiting;
  const char *lock_text;
  enum thr_lock_type type;
} TABLE_LOCK_INFO;

typedef Prealloced_array< TABLE_LOCK_INFO, 20 > Saved_locks_array;

static inline int dl_compare(const TABLE_LOCK_INFO *a, const TABLE_LOCK_INFO *b)
{
  if (a->thread_id > b->thread_id)
    return 1;
  if (a->thread_id < b->thread_id)
    return -1;
  if (a->waiting == b->waiting)
    return 0;
  else if (a->waiting)
    return -1;
  return 1;
}

class DL_commpare : public std::binary_function< const TABLE_LOCK_INFO &, const TABLE_LOCK_INFO &, bool >
{
 public:
  bool operator()(const TABLE_LOCK_INFO &a, const TABLE_LOCK_INFO &b) { return dl_compare(&a, &b) < 0; }
};

static void push_locks_into_array(Saved_locks_array *ar, THR_LOCK_DATA *data, bool wait, const char *text)
{
  if (data)
  {
    TABLE *table = (TABLE *)data->debug_print_param;
    if (table && table->s->tmp_table == NO_TMP_TABLE)
    {
      TABLE_LOCK_INFO table_lock_info;
      table_lock_info.thread_id = table->in_use->thread_id();
      memcpy(table_lock_info.table_name, table->s->table_cache_key.str, table->s->table_cache_key.length);
      table_lock_info.table_name[strlen(table_lock_info.table_name)] = '.';
      table_lock_info.waiting = wait;
      table_lock_info.lock_text = text;
      // lock_type is also obtainable from THR_LOCK_DATA
      table_lock_info.type = table->reginfo.lock_type;
      ar->push_back(table_lock_info);
    }
  }
}

/*
  Regarding MERGE tables:

  For now, the best option is to use the common TABLE *pointer for all
  cases;  The drawback is that for MERGE tables we will see many locks
  for the merge tables even if some of them are for individual tables.

  The way to solve this is to add to 'THR_LOCK' structure a pointer to
  the filename and use this when printing the data.
  (We can for now ignore this and just print the same name for all merge
  table parts;  Please add the above as a comment to the display_lock
  function so that we can easily add this if we ever need this.
*/

static void display_table_locks(void)
{
  LIST *list;
  Saved_locks_array saved_table_locks(key_memory_locked_thread_list);
  saved_table_locks.reserve(table_cache_manager.cached_tables() + 20);

  mysql_mutex_lock(&THR_LOCK_lock);
  for (list = thr_lock_thread_list; list; list = list_rest(list))
  {
    THR_LOCK *lock = (THR_LOCK *)list->data;

    mysql_mutex_lock(&lock->mutex);
    push_locks_into_array(&saved_table_locks, lock->write.data, FALSE, "Locked - write");
    push_locks_into_array(&saved_table_locks, lock->write_wait.data, TRUE, "Waiting - write");
    push_locks_into_array(&saved_table_locks, lock->read.data, FALSE, "Locked - read");
    push_locks_into_array(&saved_table_locks, lock->read_wait.data, TRUE, "Waiting - read");
    mysql_mutex_unlock(&lock->mutex);
  }
  mysql_mutex_unlock(&THR_LOCK_lock);

  if (saved_table_locks.empty())
    return;

  saved_table_locks.shrink_to_fit();

  std::sort(saved_table_locks.begin(), saved_table_locks.end(), DL_commpare());

  puts("\nThread database.table_name          Locked/Waiting        Lock_type\n");

  Saved_locks_array::iterator it;
  for (it = saved_table_locks.begin(); it != saved_table_locks.end(); ++it)
  {
    printf("%-8u%-28.28s%-22s%s\n", it->thread_id, it->table_name, it->lock_text, lock_descriptions[(int)it->type]);
  }
  puts("\n\n");
}

static int print_key_cache_status(const char *name, KEY_CACHE *key_cache)
{
  char llbuff1[22];
  char llbuff2[22];
  char llbuff3[22];
  char llbuff4[22];

  if (!key_cache->key_cache_inited)
  {
    printf("%s: Not in use\n", name);
  }
  else
  {
    printf(
        "%s\n\
Buffer_size:    %10lu\n\
Block_size:     %10lu\n\
Division_limit: %10lu\n\
Age_limit:      %10lu\n\
blocks used:    %10lu\n\
not flushed:    %10lu\n\
w_requests:     %10s\n\
writes:         %10s\n\
r_requests:     %10s\n\
reads:          %10s\n\n",
        name, (ulong)key_cache->param_buff_size, (ulong)key_cache->param_block_size,
        (ulong)key_cache->param_division_limit, (ulong)key_cache->param_age_threshold, key_cache->blocks_used,
        key_cache->global_blocks_changed, llstr(key_cache->global_cache_w_requests, llbuff1),
        llstr(key_cache->global_cache_write, llbuff2), llstr(key_cache->global_cache_r_requests, llbuff3),
        llstr(key_cache->global_cache_read, llbuff4));
  }
  return 0;
}

void mysql_print_status()
{
  char current_dir[FN_REFLEN];
  STATUS_VAR current_global_status_var;

  printf("\nStatus information:\n\n");
  (void)my_getwd(current_dir, sizeof(current_dir), MYF(0));
  printf("Current dir: %s\n", current_dir);
  printf("Running threads: %u  Stack size: %ld\n", Global_THD_manager::get_instance()->get_thd_count(),
         (long)my_thread_stack_size);
  thr_print_locks();  // Write some debug info
#ifndef NDEBUG
  print_cached_tables();
#endif
  /* Print key cache status */
  puts("\nKey caches:");
  process_key_caches(print_key_cache_status);
  mysql_mutex_lock(&LOCK_status);
  calc_sum_of_all_status(&current_global_status_var);
  printf(
      "\nhandler status:\n\
read_key:   %10llu\n\
read_next:  %10llu\n\
read_rnd    %10llu\n\
read_first: %10llu\n\
write:      %10llu\n\
delete      %10llu\n\
update:     %10llu\n",
      current_global_status_var.ha_read_key_count, current_global_status_var.ha_read_next_count,
      current_global_status_var.ha_read_rnd_count, current_global_status_var.ha_read_first_count,
      current_global_status_var.ha_write_count, current_global_status_var.ha_delete_count,
      current_global_status_var.ha_update_count);
  mysql_mutex_unlock(&LOCK_status);
  printf(
      "\nTable status:\n\
Opened tables: %10lu\n\
Open tables:   %10lu\n\
Open files:    %10lu\n\
Open streams:  %10lu\n",
      (ulong)current_global_status_var.opened_tables, (ulong)table_cache_manager.cached_tables(), my_file_opened,
      my_stream_opened);
  display_table_locks();
#ifdef HAVE_MALLOC_INFO
  printf("\nMemory status:\n");
  malloc_info(0, stdout);
#endif

#ifndef EMBEDDED_LIBRARY
  Events::dump_internal_status();
#endif
  puts("");
  fflush(stdout);
}

#ifndef NDEBUG
#ifdef EXTRA_DEBUG_DUMP_TABLE_LISTS

/*
  A fixed-size FIFO pointer queue that also doesn't allow one to put an
  element that has previously been put into it.

  There is a hard-coded limit of the total number of queue put operations.
  The implementation is trivial and is intended for use in debug dumps only.
*/

template < class T >
class Unique_fifo_queue
{
 public:
  /* Add an element to the queue */
  void push_back(T *tbl)
  {
    if (!tbl)
      return;
    // check if we've already scheduled and/or dumped the element
    for (int i = 0; i < last; i++)
    {
      if (elems[i] == tbl)
        return;
    }
    elems[last++] = tbl;
  }

  bool pop_first(T **elem)
  {
    if (first < last)
    {
      *elem = elems[first++];
      return TRUE;
    }
    return FALSE;
  }

  void reset() { first = last = 0; }
  enum
  {
    MAX_ELEMS = 1000
  };
  T *elems[MAX_ELEMS];
  int first;  // First undumped table
  int last;   // Last undumped element
};

class Dbug_table_list_dumper
{
  FILE *out;
  Unique_fifo_queue< TABLE_LIST > tables_fifo;
  Unique_fifo_queue< List< TABLE_LIST > > tbl_lists;

 public:
  void dump_one_struct(TABLE_LIST *tbl);

  int dump_graph(st_select_lex *select_lex, TABLE_LIST *first_leaf);
};

void dump_TABLE_LIST_graph(SELECT_LEX *select_lex, TABLE_LIST *tl)
{
  Dbug_table_list_dumper dumper;
  dumper.dump_graph(select_lex, tl);
}

/*
  - Dump one TABLE_LIST objects and its outgoing edges
  - Schedule that other objects seen along the edges are dumped too.
*/

void Dbug_table_list_dumper::dump_one_struct(TABLE_LIST *tbl)
{
  fprintf(out, "\"%p\" [\n", tbl);
  fprintf(out, "  label = \"%p|", tbl);
  fprintf(out, "alias=%s|", tbl->alias ? tbl->alias : "NULL");
  fprintf(out, "<next_leaf>next_leaf=%p|", tbl->next_leaf);
  fprintf(out, "<next_local>next_local=%p|", tbl->next_local);
  fprintf(out, "<next_global>next_global=%p|", tbl->next_global);
  fprintf(out, "<embedding>embedding=%p", tbl->embedding);

  if (tbl->nested_join)
    fprintf(out, "|<nested_j>nested_j=%p", tbl->nested_join);
  if (tbl->join_list)
    fprintf(out, "|<join_list>join_list=%p", tbl->join_list);
  if (tbl->on_expr)
    fprintf(out, "|<on_expr>on_expr=%p", tbl->on_expr);
  fprintf(out, "\"\n");
  fprintf(out, "  shape = \"record\"\n];\n\n");

  if (tbl->next_leaf)
  {
    fprintf(out, "\n\"%p\":next_leaf -> \"%p\"[ color = \"#000000\" ];\n", tbl, tbl->next_leaf);
    tables_fifo.push_back(tbl->next_leaf);
  }
  if (tbl->next_local)
  {
    fprintf(out, "\n\"%p\":next_local -> \"%p\"[ color = \"#404040\" ];\n", tbl, tbl->next_local);
    tables_fifo.push_back(tbl->next_local);
  }
  if (tbl->next_global)
  {
    fprintf(out, "\n\"%p\":next_global -> \"%p\"[ color = \"#808080\" ];\n", tbl, tbl->next_global);
    tables_fifo.push_back(tbl->next_global);
  }

  if (tbl->embedding)
  {
    fprintf(out, "\n\"%p\":embedding -> \"%p\"[ color = \"#FF0000\" ];\n", tbl, tbl->embedding);
    tables_fifo.push_back(tbl->embedding);
  }

  if (tbl->join_list)
  {
    fprintf(out, "\n\"%p\":join_list -> \"%p\"[ color = \"#0000FF\" ];\n", tbl, tbl->join_list);
    tbl_lists.push_back(tbl->join_list);
  }
}

int Dbug_table_list_dumper::dump_graph(st_select_lex *select_lex, TABLE_LIST *first_leaf)
{
  DBUG_ENTER("Dbug_table_list_dumper::dump_graph");
  char filename[500];
  int no = 0;
  do
  {
    sprintf(filename, "tlist_tree%.3d.g", no);
    if ((out = fopen(filename, "rt")))
    {
      /* File exists, try next name */
      fclose(out);
    }
    no++;
  } while (out);

  /* Ok, found an unoccupied name, create the file */
  if (!(out = fopen(filename, "wt")))
  {
    DBUG_PRINT("tree_dump", ("Failed to create output file"));
    DBUG_RETURN(1);
  }

  DBUG_PRINT("tree_dump", ("dumping tree to %s", filename));

  fputs("digraph g {\n", out);
  fputs("graph [", out);
  fputs("  rankdir = \"LR\"", out);
  fputs("];", out);

  TABLE_LIST *tbl;
  tables_fifo.reset();
  dump_one_struct(first_leaf);
  while (tables_fifo.pop_first(&tbl))
  {
    dump_one_struct(tbl);
  }

  List< TABLE_LIST > *plist;
  tbl_lists.push_back(&select_lex->top_join_list);
  while (tbl_lists.pop_first(&plist))
  {
    fprintf(out, "\"%p\" [\n", plist);
    fprintf(out, "  bgcolor = \"\"");
    fprintf(out, "  label = \"L %p\"", plist);
    fprintf(out, "  shape = \"record\"\n];\n\n");
  }

  fprintf(out, " { rank = same; ");
  for (TABLE_LIST *tl = first_leaf; tl; tl = tl->next_leaf) fprintf(out, " \"%p\"; ", tl);
  fprintf(out, "};\n");
  fputs("}", out);
  fclose(out);

  char filename2[500];
  filename[strlen(filename) - 1] = 0;
  filename[strlen(filename) - 1] = 0;
  sprintf(filename2, "%s.query", filename);

  if ((out = fopen(filename2, "wt")))
  {
    //    fprintf(out, "%s", current_thd->query);
    fclose(out);
  }
  DBUG_RETURN(0);
}

#endif

#endif
