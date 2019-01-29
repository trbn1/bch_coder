# -*- coding: utf-8 -*-

import numpy as np

from sympy.abc import alpha, x
from sympy import GF, lcm, Poly


def calc_gen():
    """
    Calculate generator polynomial based on a given primitive polynomial.
    """
    # Initialize primitive polynomial in GF(q).
    prim_poly = Poly(alpha ** m + alpha ** 4 + 1, alpha).set_domain(GF(q))
    print('Primitive poly: %s' % prim_poly)

    gen_poly = None
    for i in range(1, t * 2):
        # No need to calculate for even i.
        if i % 2 is not 0:
            print('Current i: %d' % i)
            # On first loop, calculate only minimal polynomial.
            if gen_poly is None:
                gen_poly = min_poly(i, prim_poly)
            # Otherwise calculate lowest common multiplier using current value
            # of generator polynomial and minimal polynomial with current i.
            else:
                gen_poly = lcm(gen_poly, min_poly(i, prim_poly))

    # Truncate to GF(q).
    gen_poly = gen_poly.trunc(q)
    print('Generator poly: %s' % gen_poly)
    return prim_poly, gen_poly


def min_poly(i, prim_poly):
    """
    Calculate minimal polynomial for a given i and primitive polynomial.
    """
    checked = np.zeros(n, dtype=bool)
    checked[i] = True
    poly = Poly(x - alpha ** i, x)
    for j in range(n):
        i = (i * q) % n
        if checked[i]:
            polys = [(Poly(c, alpha) % prim_poly).trunc(q) for c in poly.all_coeffs()]
            for p in polys:
                if p.degree() > 0:
                    raise Exception('Couldnt find minimal polynomial')
            coeffs = [p.nth(0) for p in polys]
            return Poly(coeffs, x)
        poly = poly * Poly(x - alpha ** i, x)
    return None
    
def output_to_file():
    """
    Output all parameters to a text file.
    """
    with open('gen.log', 'w') as file:
        file.write('q: ' + hex(q) + '\n')
        file.write('m: ' + hex(m) + '\n')
        file.write('n: ' + hex(n) + '\n')
        file.write('k: ' + hex(k) + '\n')
        file.write('t: ' + hex(t) + '\n')
        file.write('d: ' + hex(d) + '\n')
        file.write('l: ' + hex(l) + '\n')
        tmp2 = prim_poly.all_coeffs()[::-1]
        tmp = ''
        count = 0
        for i in tmp2:
            if i == 1:
                tmp += hex(count) + ', '
            count += 1
        file.write('\nPrimitive poly:\n' + tmp + '\n')

        count = 0
        coeff = 0
        tmp2 = gen_poly.all_coeffs()[::-1]
        tmp = ''
        for i in tmp2:
            if i == 1:
                tmp += hex(count) + ', '
                coeff += 1
            count += 1
            if coeff % 8 == 0:
                tmp += '\n'
        file.write('\nGenerator poly:\n' + tmp)
        return None


if __name__ == '__main__':
    q = 2
    m = 9
    n = 511
    k = 220
    t = 39
    d = 2 * t + 1
    l = n - k
    print('Calculating generator with parameters:\nq = %d\nm = %d\nn = %d\nk = %d\nt = %d' % (q, m, n, k, t))
    prim_poly, gen_poly = calc_gen()
    output_to_file()