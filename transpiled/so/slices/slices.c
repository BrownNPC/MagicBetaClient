#include "slices.h"

// -- Types --

// hint for pdqsort when choosing the pivot
typedef so_int sortedHint;

// xorshift paper: https://www.jstatsoft.org/article/view/v008i14/xorshift.pdf
typedef uint64_t xorshift;

// -- Forward declarations --
static uint64_t xorshift_Next(void* self);
static so_uint nextPowerOfTwo(so_int length);
static void insertionSort_func(slices_Sorter data, so_int a, so_int b);
static void siftDown_func(slices_Sorter data, so_int lo, so_int hi, so_int first);
static void heapSort_func(slices_Sorter data, so_int a, so_int b);
static void pdqsort_func(slices_Sorter data, so_int a, so_int b, so_int limit);
static so_R_int_bool partition_func(slices_Sorter data, so_int a, so_int b, so_int pivot);
static so_int partitionEqual_func(slices_Sorter data, so_int a, so_int b, so_int pivot);
static bool partialInsertionSort_func(slices_Sorter data, so_int a, so_int b);
static void breakPatterns_func(slices_Sorter data, so_int a, so_int b);
static so_R_int_int choosePivot_func(slices_Sorter data, so_int a, so_int b);
static so_R_int_int order2_func(slices_Sorter data, so_int a, so_int b, so_int* swaps);
static so_int median_func(slices_Sorter data, so_int a, so_int b, so_int c, so_int* swaps);
static so_int medianAdjacent_func(slices_Sorter data, so_int a, so_int* swaps);
static void reverseRange_func(slices_Sorter data, so_int a, so_int b);
static void swapRange_func(slices_Sorter data, so_int a, so_int b, so_int n);
static void stable_func(slices_Sorter data, so_int n);
static void symMerge_func(slices_Sorter data, so_int a, so_int m, so_int b);
static void rotate_func(slices_Sorter data, so_int a, so_int m, so_int b);

// -- Variables and constants --
static const sortedHint unknownHint = 0;
static const sortedHint increasingHint = 1;
static const sortedHint decreasingHint = 2;

// -- append.go --

// -- slices.go --

// -- sort.go --

// Compare compares the elements at indices i and j.
// Returns a negative value if s[i] < s[j], zero if they are equal,
// and a positive value if s[i] > s[j].
so_int slices_Sorter_Compare(slices_Sorter s, so_int i, so_int j) {
    so_byte* a = c_PtrAdd(so_byte, (s.slice.ptr), (i * s.esize));
    so_byte* b = c_PtrAdd(so_byte, (s.slice.ptr), (j * s.esize));
    if (s.compare != NULL) {
        return s.compare(a, b);
    }
    return mem_Compare(a, b, s.esize);
}

// Less reports whether the element at index i
// should sort before the element at index j.
bool slices_Sorter_Less(slices_Sorter s, so_int i, so_int j) {
    return slices_Sorter_Compare(s, i, j) < 0;
}

// Swap swaps the elements at indices i and j.
void slices_Sorter_Swap(slices_Sorter s, so_int i, so_int j) {
    so_byte* a = c_PtrAdd(so_byte, (s.slice.ptr), (i * s.esize));
    so_byte* b = c_PtrAdd(so_byte, (s.slice.ptr), (j * s.esize));
    mem_SwapByte(a, b, s.esize);
}

// SortWith sorts the slice using the provided Sorter.
void slices_SortWith(slices_Sorter s) {
    so_int limit = bits_Len((so_uint)(s.slice.len));
    pdqsort_func(s, 0, s.slice.len, limit);
}

// SortStableWith sorts the slice using the provided Sorter
// while keeping the original order of equal elements.
void slices_SortStableWith(slices_Sorter s) {
    stable_func(s, s.slice.len);
}

// IsSortedWith reports whether the slice is sorted
// according to the provided Sorter.
bool slices_IsSortedWith(slices_Sorter s) {
    for (so_int i = s.slice.len - 1; i > 0; i--) {
        if (slices_Sorter_Compare(s, i, i - 1) < 0) {
            return false;
        }
    }
    return true;
}

// -- sortfunc.go --

static uint64_t xorshift_Next(void* self) {
    xorshift* r = self;
    *r ^= (*r << 13);
    *r ^= (*r >> 7);
    *r ^= (*r << 17);
    return (uint64_t)(*r);
}

static so_uint nextPowerOfTwo(so_int length) {
    so_uint shift = (so_uint)(bits_Len((so_uint)(length)));
    return (so_uint)((so_uint)1 << shift);
}

// insertionSort_func sorts data[a:b] using insertion sort.
static void insertionSort_func(slices_Sorter data, so_int a, so_int b) {
    for (so_int i = a + 1; i < b; i++) {
        for (so_int j = i; j > a && slices_Sorter_Less(data, j, j - 1); j--) {
            slices_Sorter_Swap(data, j, j - 1);
        }
    }
}

// siftDown_func implements the heap property on data[lo:hi].
// first is an offset into the array where the root of the heap lies.
static void siftDown_func(slices_Sorter data, so_int lo, so_int hi, so_int first) {
    so_int root = lo;
    for (;;) {
        so_int child = 2 * root + 1;
        if (child >= hi) {
            break;
        }
        if (child + 1 < hi && slices_Sorter_Less(data, first + child, first + child + 1)) {
            child++;
        }
        if (!slices_Sorter_Less(data, first + root, first + child)) {
            return;
        }
        slices_Sorter_Swap(data, first + root, first + child);
        root = child;
    }
}

static void heapSort_func(slices_Sorter data, so_int a, so_int b) {
    so_int first = a;
    so_int lo = 0;
    so_int hi = b - a;
    // Build heap with greatest element at top.
    for (so_int i = (hi - 1) / 2; i >= 0; i--) {
        siftDown_func(data, i, hi, first);
    }
    // Pop elements, largest first, into end of data.
    for (so_int i = hi - 1; i >= 0; i--) {
        slices_Sorter_Swap(data, first, first + i);
        siftDown_func(data, lo, i, first);
    }
}

// pdqsort_func sorts data[a:b].
// The algorithm based on pattern-defeating quicksort(pdqsort), but without the optimizations from BlockQuicksort.
// pdqsort paper: https://arxiv.org/pdf/2106.05123.pdf
// C++ implementation: https://github.com/orlp/pdqsort
// Rust implementation: https://docs.rs/pdqsort/latest/pdqsort/
// limit is the number of allowed bad (very unbalanced) pivots before falling back to heapsort.
static void pdqsort_func(slices_Sorter data, so_int a, so_int b, so_int limit) {
    const int64_t maxInsertion = 12;
    bool wasBalanced = true;
    bool wasPartitioned = true;
    for (;;) {
        so_int length = b - a;
        if (length <= maxInsertion) {
            insertionSort_func(data, a, b);
            return;
        }
        // Fall back to heapsort if too many bad choices were made.
        if (limit == 0) {
            heapSort_func(data, a, b);
            return;
        }
        // If the last partitioning was imbalanced, we need to breaking patterns.
        if (!wasBalanced) {
            breakPatterns_func(data, a, b);
            limit--;
        }
        so_R_int_int _res1 = choosePivot_func(data, a, b);
        so_int pivot = _res1.val;
        sortedHint hint = _res1.val2;
        if (hint == decreasingHint) {
            reverseRange_func(data, a, b);
            // The chosen pivot was pivot-a elements after the start of the array.
            // After reversing it is pivot-a elements before the end of the array.
            // The idea came from Rust's implementation.
            pivot = (b - 1) - (pivot - a);
            hint = increasingHint;
        }
        // The slice is likely already sorted.
        if (wasBalanced && wasPartitioned && hint == increasingHint) {
            if (partialInsertionSort_func(data, a, b)) {
                return;
            }
        }
        // Probably the slice contains many duplicate elements, partition the slice into
        // elements equal to and elements greater than the pivot.
        if (a > 0 && !slices_Sorter_Less(data, a - 1, pivot)) {
            so_int mid = partitionEqual_func(data, a, b, pivot);
            a = mid;
            continue;
        }
        so_R_int_bool _res2 = partition_func(data, a, b, pivot);
        so_int mid = _res2.val;
        bool alreadyPartitioned = _res2.val2;
        wasPartitioned = alreadyPartitioned;
        so_int leftLen = mid - a, rightLen = b - mid;
        so_int balanceThreshold = length / 8;
        if (leftLen < rightLen) {
            wasBalanced = leftLen >= balanceThreshold;
            pdqsort_func(data, a, mid, limit);
            a = mid + 1;
        } else {
            wasBalanced = rightLen >= balanceThreshold;
            pdqsort_func(data, mid + 1, b, limit);
            b = mid;
        }
    }
}

// partition_func does one quicksort partition.
// Let p = data[pivot]
// Moves elements in data[a:b] around, so that data[i]<p and data[j]>=p for i<newpivot and j>newpivot.
// On return, data[newpivot] = p
// Returns (newpivot, alreadyPartitioned).
static so_R_int_bool partition_func(slices_Sorter data, so_int a, so_int b, so_int pivot) {
    slices_Sorter_Swap(data, a, pivot);
    // i and j are inclusive of the elements remaining to be partitioned
    so_int i = a + 1, j = b - 1;
    for (; i <= j && slices_Sorter_Less(data, i, a);) {
        i++;
    }
    for (; i <= j && !slices_Sorter_Less(data, j, a);) {
        j--;
    }
    if (i > j) {
        slices_Sorter_Swap(data, j, a);
        return (so_R_int_bool){.val = j, .val2 = true};
    }
    slices_Sorter_Swap(data, i, j);
    i++;
    j--;
    for (;;) {
        for (; i <= j && slices_Sorter_Less(data, i, a);) {
            i++;
        }
        for (; i <= j && !slices_Sorter_Less(data, j, a);) {
            j--;
        }
        if (i > j) {
            break;
        }
        slices_Sorter_Swap(data, i, j);
        i++;
        j--;
    }
    slices_Sorter_Swap(data, j, a);
    return (so_R_int_bool){.val = j, .val2 = false};
}

// partitionEqual_func partitions data[a:b] into elements equal to data[pivot] followed by elements greater than data[pivot].
// It assumed that data[a:b] does not contain elements smaller than the data[pivot].
// Returns newpivot.
static so_int partitionEqual_func(slices_Sorter data, so_int a, so_int b, so_int pivot) {
    slices_Sorter_Swap(data, a, pivot);
    // i and j are inclusive of the elements remaining to be partitioned
    so_int i = a + 1, j = b - 1;
    for (;;) {
        for (; i <= j && !slices_Sorter_Less(data, a, i);) {
            i++;
        }
        for (; i <= j && slices_Sorter_Less(data, a, j);) {
            j--;
        }
        if (i > j) {
            break;
        }
        slices_Sorter_Swap(data, i, j);
        i++;
        j--;
    }
    return i;
}

// partialInsertionSort_func partially sorts a slice, returns true if the slice is sorted at the end.
static bool partialInsertionSort_func(slices_Sorter data, so_int a, so_int b) {
    const int64_t maxSteps = 5;
    const int64_t shortestShifting = 50;
    so_int i = a + 1;
    for (so_int j = 0; j < maxSteps; j++) {
        for (; i < b && !slices_Sorter_Less(data, i, i - 1);) {
            i++;
        }
        if (i == b) {
            return true;
        }
        if (b - a < shortestShifting) {
            return false;
        }
        slices_Sorter_Swap(data, i, i - 1);
        // Shift the smaller one to the left.
        if (i - a >= 2) {
            for (so_int j = i - 1; j >= 1; j--) {
                if (!slices_Sorter_Less(data, j, j - 1)) {
                    break;
                }
                slices_Sorter_Swap(data, j, j - 1);
            }
        }
        // Shift the greater one to the right.
        if (b - i >= 2) {
            for (so_int j = i + 1; j < b; j++) {
                if (!slices_Sorter_Less(data, j, j - 1)) {
                    break;
                }
                slices_Sorter_Swap(data, j, j - 1);
            }
        }
    }
    return false;
}

// breakPatterns_func scatters some elements around in an attempt to break some patterns
// that might cause imbalanced partitions in quicksort.
static void breakPatterns_func(slices_Sorter data, so_int a, so_int b) {
    so_int length = b - a;
    if (length >= 8) {
        xorshift random = (xorshift)(length);
        so_uint modulus = nextPowerOfTwo(length);
        for (so_int idx = a + (length / 4) * 2 - 1; idx <= a + (length / 4) * 2 + 1; idx++) {
            so_int other = (so_int)((so_uint)(xorshift_Next(&random)) & (modulus - 1));
            if (other >= length) {
                other -= length;
            }
            slices_Sorter_Swap(data, idx, a + other);
        }
    }
}

// choosePivot_func chooses a pivot in data[a:b].
//
// [0,8): chooses a static pivot.
// [8,shortestNinther): uses the simple median-of-three method.
// [shortestNinther,∞): uses the Tukey ninther method.
//
// Return (pivot, hint)
static so_R_int_int choosePivot_func(slices_Sorter data, so_int a, so_int b) {
    const int64_t shortestNinther = 50;
    const int64_t maxSwaps = 4 * 3;
    so_int l = b - a;
    so_int swaps = 0;
    so_int i = a + l / 4 * 1;
    so_int j = a + l / 4 * 2;
    so_int k = a + l / 4 * 3;
    if (l >= 8) {
        if (l >= shortestNinther) {
            // Tukey ninther method, the idea came from Rust's implementation.
            i = medianAdjacent_func(data, i, &swaps);
            j = medianAdjacent_func(data, j, &swaps);
            k = medianAdjacent_func(data, k, &swaps);
        }
        // Find the median among i, j, k and stores it into j.
        j = median_func(data, i, j, k, &swaps);
    }
    if (swaps == (0)) {
        return (so_R_int_int){.val = j, .val2 = increasingHint};
    } else if (swaps == (maxSwaps)) {
        return (so_R_int_int){.val = j, .val2 = decreasingHint};
    } else {
        return (so_R_int_int){.val = j, .val2 = unknownHint};
    }
}

// order2_func returns x,y where data[x] <= data[y], where x,y=a,b or x,y=b,a.
static so_R_int_int order2_func(slices_Sorter data, so_int a, so_int b, so_int* swaps) {
    if (slices_Sorter_Less(data, b, a)) {
        *swaps = *swaps + 1;
        return (so_R_int_int){.val = b, .val2 = a};
    }
    return (so_R_int_int){.val = a, .val2 = b};
}

// median_func returns x where data[x] is the median of data[a],data[b],data[c], where x is a, b, or c.
static so_int median_func(slices_Sorter data, so_int a, so_int b, so_int c, so_int* swaps) {
    so_R_int_int _res1 = order2_func(data, a, b, swaps);
    a = _res1.val;
    b = _res1.val2;
    so_R_int_int _res2 = order2_func(data, b, c, swaps);
    b = _res2.val;
    c = _res2.val2;
    so_R_int_int _res3 = order2_func(data, a, b, swaps);
    a = _res3.val;
    b = _res3.val2;
    return b;
}

// medianAdjacent_func finds the median of data[a - 1], data[a], data[a + 1] and stores the index into a.
static so_int medianAdjacent_func(slices_Sorter data, so_int a, so_int* swaps) {
    return median_func(data, a - 1, a, a + 1, swaps);
}

static void reverseRange_func(slices_Sorter data, so_int a, so_int b) {
    so_int i = a;
    so_int j = b - 1;
    for (; i < j;) {
        slices_Sorter_Swap(data, i, j);
        i++;
        j--;
    }
}

static void swapRange_func(slices_Sorter data, so_int a, so_int b, so_int n) {
    for (so_int i = 0; i < n; i++) {
        slices_Sorter_Swap(data, a + i, b + i);
    }
}

static void stable_func(slices_Sorter data, so_int n) {
    // must be > 0
    so_int blockSize = 20;
    so_int a = 0, b = blockSize;
    for (; b <= n;) {
        insertionSort_func(data, a, b);
        a = b;
        b += blockSize;
    }
    insertionSort_func(data, a, n);
    for (; blockSize < n;) {
        a = 0;
        b = 2 * blockSize;
        for (; b <= n;) {
            symMerge_func(data, a, a + blockSize, b);
            a = b;
            b += 2 * blockSize;
        }
        {
            so_int m = a + blockSize;
            if (m < n) {
                symMerge_func(data, a, m, n);
            }
        }
        blockSize *= 2;
    }
}

// symMerge_func merges the two sorted subsequences data[a:m] and data[m:b] using
// the SymMerge algorithm from Pok-Son Kim and Arne Kutzner, "Stable Minimum
// Storage Merging by Symmetric Comparisons", in Susanne Albers and Tomasz
// Radzik, editors, Algorithms - ESA 2004, volume 3221 of Lecture Notes in
// Computer Science, pages 714-723. Springer, 2004.
//
// Let M = m-a and N = b-n. Wolog M < N.
// The recursion depth is bound by ceil(log(N+M)).
// The algorithm needs O(M*log(N/M + 1)) calls to data.Less.
// The algorithm needs O((M+N)*log(M)) calls to data.Swap.
//
// The paper gives O((M+N)*log(M)) as the number of assignments assuming a
// rotation algorithm which uses O(M+N+gcd(M+N)) assignments. The argumentation
// in the paper carries through for Swap operations, especially as the block
// swapping rotate uses only O(M+N) Swaps.
//
// symMerge assumes non-degenerate arguments: a < m && m < b.
// Having the caller check this condition eliminates many leaf recursion calls,
// which improves performance.
static void symMerge_func(slices_Sorter data, so_int a, so_int m, so_int b) {
    // Avoid unnecessary recursions of symMerge
    // by direct insertion of data[a] into data[m:b]
    // if data[a:m] only contains one element.
    if (m - a == 1) {
        // Use binary search to find the lowest index i
        // such that data[i] >= data[a] for m <= i < b.
        // Exit the search loop with i == b in case no such index exists.
        so_int i = m;
        so_int j = b;
        for (; i < j;) {
            so_int h = (so_int)((so_uint)(i + j) >> 1);
            if (slices_Sorter_Less(data, h, a)) {
                i = h + 1;
            } else {
                j = h;
            }
        }
        // Swap values until data[a] reaches the position before i.
        for (so_int k = a; k < i - 1; k++) {
            slices_Sorter_Swap(data, k, k + 1);
        }
        return;
    }
    // Avoid unnecessary recursions of symMerge
    // by direct insertion of data[m] into data[a:m]
    // if data[m:b] only contains one element.
    if (b - m == 1) {
        // Use binary search to find the lowest index i
        // such that data[i] > data[m] for a <= i < m.
        // Exit the search loop with i == m in case no such index exists.
        so_int i = a;
        so_int j = m;
        for (; i < j;) {
            so_int h = (so_int)((so_uint)(i + j) >> 1);
            if (!slices_Sorter_Less(data, m, h)) {
                i = h + 1;
            } else {
                j = h;
            }
        }
        // Swap values until data[m] reaches the position i.
        for (so_int k = m; k > i; k--) {
            slices_Sorter_Swap(data, k, k - 1);
        }
        return;
    }
    so_int mid = (so_int)((so_uint)(a + b) >> 1);
    so_int n = mid + m;
    so_int start = 0, r = 0;
    if (m > mid) {
        start = n - b;
        r = mid;
    } else {
        start = a;
        r = m;
    }
    so_int p = n - 1;
    for (; start < r;) {
        so_int c = (so_int)((so_uint)(start + r) >> 1);
        if (!slices_Sorter_Less(data, p - c, c)) {
            start = c + 1;
        } else {
            r = c;
        }
    }
    so_int end = n - start;
    if (start < m && m < end) {
        rotate_func(data, start, m, end);
    }
    if (a < start && start < mid) {
        symMerge_func(data, a, start, mid);
    }
    if (mid < end && end < b) {
        symMerge_func(data, mid, end, b);
    }
}

// rotate_func rotates two consecutive blocks u = data[a:m] and v = data[m:b] in data:
// Data of the form 'x u v y' is changed to 'x v u y'.
// rotate performs at most b-a many calls to data.Swap,
// and it assumes non-degenerate arguments: a < m && m < b.
static void rotate_func(slices_Sorter data, so_int a, so_int m, so_int b) {
    so_int i = m - a;
    so_int j = b - m;
    for (; i != j;) {
        if (i > j) {
            swapRange_func(data, m - i, m, j);
            i -= j;
        } else {
            swapRange_func(data, m - i, m + j - i, i);
            j -= i;
        }
    }
    // i == j
    swapRange_func(data, m - i, m, i);
}
