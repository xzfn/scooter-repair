

if __name__ == '__main__':
    txt = '../logs/serial-20250608-205905_readable.txt'
    focs = []
    dashes = []
    last_is_foc = False
    with open(txt, 'r') as f:
        for line in f:
            if '(<-FOC) 20 82' in line:
                b = line.rsplit(' ', 3)[-3]
                foc_speed = int(b, 16)
                if not last_is_foc:
                    focs.append(foc_speed)
                    last_is_foc = True
            elif '(->Dash) 10 04' in line:
                data = line.split(' (->Dash) ')[1]
                b = data.split(' ', 5)[5]
                dash_speed = int(data.split(' ', 6)[5], 16)
                if last_is_foc:
                    dashes.append(dash_speed)
                    last_is_foc = False

    print(len(focs), len(dashes))
    pairs = {}
    for foc_speed, dash_speed in zip(focs, dashes):
        pairs[foc_speed] = dash_speed

    print(pairs)
    pairs = dict(sorted(pairs.items()))
    print(pairs)
    # dash_speed = foc_speed * 12 // 25
    for foc_speed, dash_speed in pairs.items():
        print(foc_speed * 12 // 25 == dash_speed)
