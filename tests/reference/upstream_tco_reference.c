/* Test-only integer minor-unit TCO reference; not a billing or pricing engine. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum result { TCO_OK, TCO_UNKNOWN, TCO_INVALID, TCO_OVERFLOW };
enum { ONE_TIME = 3, MONTHLY = 4, MAX_MONTHS = 120 };
struct amount { uint64_t value; int known; };
struct option {
    char currency[4];
    unsigned months;
    struct amount once[ONE_TIME]; /* integration, validation, compliance */
    struct amount month[MONTHLY]; /* maintenance, CI, operations, remediation */
};
struct comparison { uint64_t baseline, candidate, difference; int candidate_lower; };

static int valid_currency(const char code[4])
{
    for (unsigned i = 0; i < 3; ++i)
        if (code[i] < 'A' || code[i] > 'Z') return 0;
    return code[3] == '\0';
}

static enum result add_amount(uint64_t *sum, struct amount a)
{
    if (a.known != 0 && a.known != 1) return TCO_INVALID;
    if (!a.known) return a.value == 0 ? TCO_UNKNOWN : TCO_INVALID;
    if (a.value > UINT64_MAX - *sum) return TCO_OVERFLOW;
    *sum += a.value;
    return TCO_OK;
}

static enum result total(const struct option *o, uint64_t *out)
{
    if (!o || !out || !valid_currency(o->currency) ||
        o->months == 0 || o->months > MAX_MONTHS) return TCO_INVALID;
    uint64_t once = 0, month = 0;
    for (unsigned i = 0; i < ONE_TIME; ++i) {
        enum result rc = add_amount(&once, o->once[i]);
        if (rc != TCO_OK) return rc;
    }
    for (unsigned i = 0; i < MONTHLY; ++i) {
        enum result rc = add_amount(&month, o->month[i]);
        if (rc != TCO_OK) return rc;
    }
    if (month > (UINT64_MAX - once) / o->months) return TCO_OVERFLOW;
    *out = once + month * o->months;
    return TCO_OK;
}

static enum result compare(const struct option *a, const struct option *b,
                           struct comparison *out)
{
    if (!a || !b || !out || !valid_currency(a->currency) ||
        !valid_currency(b->currency) || memcmp(a->currency, b->currency, 4) ||
        a->months != b->months) return TCO_INVALID;
    uint64_t x, y;
    enum result rc = total(a, &x);
    if (rc != TCO_OK) return rc;
    rc = total(b, &y);
    if (rc != TCO_OK) return rc;
    *out = (struct comparison){x, y, x >= y ? x-y : y-x, y < x};
    return TCO_OK;
}

/* Evaluate every integer horizon in the bounded planning range. No
 * extrapolation beyond the range, discounting, or financial recommendation. */
struct sensitivity {
    unsigned lower, equal, higher, first_lower;
};
static enum result sweep(const struct option *a, const struct option *b,
                         unsigned limit, struct sensitivity *out)
{
    if (!a || !b || !out || limit == 0 || limit > MAX_MONTHS ||
        a->months != b->months || a->months == 0 || a->months > MAX_MONTHS)
        return TCO_INVALID;
    struct option x = *a, y = *b;
    struct sensitivity result = {0};
    for (unsigned h = 1; h <= limit; ++h) {
        struct comparison c;
        x.months = y.months = h;
        enum result rc = compare(&x, &y, &c);
        if (rc != TCO_OK) return rc;
        if (c.difference == 0) ++result.equal;
        else if (c.candidate_lower) {
            ++result.lower;
            if (result.first_lower == 0) result.first_lower = h;
        } else ++result.higher;
    }
    *out = result;
    return TCO_OK;
}

static struct option fixture(void)
{
    struct option o = { .currency = "INR", .months = 12,
        .once = {{1000,1},{200,1},{100,1}},
        .month = {{100,1},{50,1},{20,1},{30,1}} };
    return o;
}

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "TCO fixture failed at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    struct option a = fixture(), b = fixture();
    struct comparison c = {0};
    CHECK(compare(&a,&b,&c)==TCO_OK && c.baseline==3700 && c.difference==0 && !c.candidate_lower);
    b.month[0].value=50;
    CHECK(compare(&a,&b,&c)==TCO_OK && c.candidate==3100 && c.difference==600 && c.candidate_lower);
    CHECK(compare(&b,&a,&c)==TCO_OK && c.difference==600 && !c.candidate_lower);
    b.month[1]=(struct amount){0,0};
    CHECK(compare(&a,&b,&c)==TCO_UNKNOWN && c.baseline==3100 && c.candidate==3700);
    b=fixture(); b.month[1]=(struct amount){1,0};
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    b=fixture(); b.month[1].known=2;
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    b=fixture(); b.month[1].value=0;
    CHECK(compare(&a,&b,&c)==TCO_OK && c.candidate==3100);
    b=fixture(); b.months=13;
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    b=fixture(); memcpy(b.currency,"USD",4);
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    b=fixture(); b.currency[3]='X';
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    a=fixture(); b=a; a.months=b.months=0;
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    a.months=b.months=121;
    CHECK(compare(&a,&b,&c)==TCO_INVALID);
    a=fixture(); b=a; b.once[0].value=UINT64_MAX;
    CHECK(compare(&a,&b,&c)==TCO_OVERFLOW);
    b=a; b.month[0].value=UINT64_MAX;
    CHECK(compare(&a,&b,&c)==TCO_OVERFLOW);
    b=a; b.month[0].value=UINT64_MAX/12;
    CHECK(compare(&a,&b,&c)==TCO_OVERFLOW);
    CHECK(compare(NULL,&b,&c)==TCO_INVALID);
    CHECK(compare(&a,&b,NULL)==TCO_INVALID);
    printf("upstream_tco_reference=PASS scenarios=17 synthetic_only=1\n");

    a=fixture(); b=a;
    struct sensitivity s = {0};
    CHECK(sweep(&a,&b,120,&s)==TCO_OK && s.equal==120 &&
          s.lower==0 && s.higher==0 && s.first_lower==0);
    /* Initial premium 600, monthly savings 50: equal at month 12. */
    b.once[0].value+=600; b.month[0].value-=50;
    CHECK(sweep(&a,&b,120,&s)==TCO_OK && s.higher==11 &&
          s.equal==1 && s.lower==108 && s.first_lower==13);
    CHECK(sweep(&b,&a,120,&s)==TCO_OK && s.lower==11 &&
          s.equal==1 && s.higher==108 && s.first_lower==1);
    CHECK(sweep(&a,&b,11,&s)==TCO_OK && s.higher==11 &&
          s.lower==0 && s.equal==0 && s.first_lower==0);
    b.once[0].value+=1;
    CHECK(sweep(&a,&b,120,&s)==TCO_OK && s.higher==12 &&
          s.equal==0 && s.lower==108 && s.first_lower==13);
    b=a; b.month[0].value=0;
    CHECK(sweep(&a,&b,120,&s)==TCO_OK && s.lower==120 &&
          s.equal==0 && s.higher==0 && s.first_lower==1);
    /* A later arithmetic failure must not publish a partial sweep. */
    b=a; b.once[0].value=0;
    b.month[0].value=UINT64_MAX/2;
    CHECK(sweep(&a,&b,3,&s)==TCO_OVERFLOW && s.lower==120 &&
          s.equal==0 && s.higher==0 && s.first_lower==1);
    b=a; b.month[0]=(struct amount){0,0};
    CHECK(sweep(&a,&b,120,&s)==TCO_UNKNOWN && s.lower==120);
    b=a; b.months=13;
    CHECK(sweep(&a,&b,120,&s)==TCO_INVALID);
    b=a;
    CHECK(sweep(&a,&b,0,&s)==TCO_INVALID);
    CHECK(sweep(&a,&b,121,&s)==TCO_INVALID);
    CHECK(sweep(&a,&b,120,NULL)==TCO_INVALID);
    printf("upstream_tco_sensitivity=PASS scenarios=12 max_months=120 synthetic_only=1\n");
    return 0;
}
