#include "postgres.h"

#include "executor/executor.h"
#include "tcop/utility.h"
#include "utils/guc.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

/* GUC variables */
static bool	pgat_log_nested_statements = false;

/* Current nesting depth of ExecutorRun+ProcessUtility calls */
static int	nested_level = 0;

#define log_statements_enabled()	\
	(nested_level == 0 || pgat_log_nested_statements)

static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorRun_hook_type prev_ExecutorRun = NULL;
static ExecutorFinish_hook_type prev_ExecutorFinish = NULL;
static ProcessUtility_hook_type prev_ProcessUtility = NULL;

static void pgat_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void pgat_ExecutorRun(QueryDesc *queryDesc,
				 ScanDirection direction,
				 long count);
static void pgat_ExecutorFinish(QueryDesc *queryDesc);
static void pgat_ProcessUtility(Node *parsetree, const char *queryString,
					ProcessUtilityContext context, ParamListInfo params,
					DestReceiver *dest, char *completionTag);

/*
 * Module load callback
 */
void
_PG_init(void)
{
	/* Define custom GUC variables. */
	DefineCustomBoolVariable("pg_audit_trail.log_nested_statements",
							 "Log nested statements.",
							 NULL,
							 &pgat_log_nested_statements,
							 false,
							 PGC_SUSET,
							 0,
							 NULL,
							 NULL,
							 NULL);

	EmitWarningsOnPlaceholders("pg_audit_trail");

	/*
	 * Install hooks.
	 */
	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = pgat_ExecutorStart;
	prev_ExecutorRun = ExecutorRun_hook;
	ExecutorRun_hook = pgat_ExecutorRun;
	prev_ExecutorFinish = ExecutorFinish_hook;
	ExecutorFinish_hook = pgat_ExecutorFinish;
	prev_ProcessUtility = ProcessUtility_hook;
	ProcessUtility_hook = pgat_ProcessUtility;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	ExecutorStart_hook = prev_ExecutorStart;
	ExecutorRun_hook = prev_ExecutorRun;
	ExecutorFinish_hook = prev_ExecutorFinish;
	ProcessUtility_hook = prev_ProcessUtility;
}

static void
pgat_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	if (log_statements_enabled())
	{
		ereport(LOG,
				(errmsg("audit: %s", queryDesc->sourceText),
				 errhidestmt(true)));
	}

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
}

static void
pgat_ExecutorRun(QueryDesc *queryDesc, ScanDirection direction, long count)
{
	nested_level++;
	PG_TRY();
	{
		if (prev_ExecutorRun)
			prev_ExecutorRun(queryDesc, direction, count);
		else
			standard_ExecutorRun(queryDesc, direction, count);
		nested_level--;
	}
	PG_CATCH();
	{
		nested_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();
}

static void
pgat_ExecutorFinish(QueryDesc *queryDesc)
{
	nested_level++;
	PG_TRY();
	{
		if (prev_ExecutorFinish)
			prev_ExecutorFinish(queryDesc);
		else
			standard_ExecutorFinish(queryDesc);
		nested_level--;
	}
	PG_CATCH();
	{
		nested_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();
}

static void
pgat_ProcessUtility(Node *parsetree, const char *queryString,
					ProcessUtilityContext context, ParamListInfo params,
					DestReceiver *dest, char *completionTag)
{
	if (log_statements_enabled())
	{
		ereport(LOG,
				(errmsg("audit: %s", queryString),
				 errhidestmt(true)));
	}

	nested_level++;
	PG_TRY();
	{
		if (prev_ProcessUtility)
			prev_ProcessUtility(parsetree, queryString,
								context, params,
								dest, completionTag);
		else
			standard_ProcessUtility(parsetree, queryString,
									context, params,
									dest, completionTag);
		nested_level--;
	}
	PG_CATCH();
	{
		nested_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();
}
