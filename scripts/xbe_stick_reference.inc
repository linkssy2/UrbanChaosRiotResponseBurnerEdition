// Project-authored keyboard movement reference retained by the input fixture.
// These helpers are an oracle for diagonal normalization, not runtime input.
static float ucrr_host_stick_value(int value)
{
    /* Preserve direction; -32768 is the asymmetric native endpoint. */
    return value < 0 ? value / 32768.0f : value / 32767.0f;
}

static float ucrr_host_stick_axis(int x, int y, unsigned component, unsigned deadzone)
{
    /* Host-controller policy, not a claim about the original Xbox response
     * curve: circular XInput dead zone, then continuous range rescaling. */
    float a = ucrr_host_stick_value(x), b = ucrr_host_stick_value(y);
    float length = sqrtf(a*a + b*b), threshold = deadzone / 32767.0f;
    if (length <= threshold) return 0;
    float strength = ((length > 1 ? 1 : length) - threshold) / (1 - threshold);
    return (component ? b : a) / length * strength;
}
