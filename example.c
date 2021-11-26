#define TM_START(tid, ro)               {sigjmp_buf *_e = stm_start(_a); if (_e != NULL) sigsetjmp(*_e, 0)
#define TM_LOAD(addr)                   stm_load((stm_word_t *)addr)
#define TM_STORE(addr, value)           stm_store((stm_word_t *)addr, (stm_word_t)value)
#define TM_COMMIT                       stm_commit(); }








static int transfer(account_t *src, account_t *dst, int amount)
{
  long i;

  /* Allow overdrafts */
  TM_START(0, RW);
  i = TM_LOAD(&src->balance);
  i -= amount;
  TM_STORE(&src->balance, i);
  i = TM_LOAD(&dst->balance);
  i += amount;
  TM_STORE(&dst->balance, i);
  TM_COMMIT;


  return amount;
}



