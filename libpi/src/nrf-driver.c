#include "nrf.h"
#include "nrf-hw-support.h"

// enable crc, enable 2 byte
#   define set_bit(x) (1<<(x))
#   define enable_crc      set_bit(3)
#   define crc_two_byte    set_bit(2)
#   define mask_int         set_bit(6)|set_bit(5)|set_bit(4)
enum {
    // pre-computed: can write into NRF_CONFIG to enable TX.
    tx_config = enable_crc | crc_two_byte | set_bit(PWR_UP) | mask_int,
    // pre-computed: can write into NRF_CONFIG to enable RX.
    rx_config = tx_config  | set_bit(PRIM_RX)
} ;

nrf_t * nrf_init(nrf_conf_t c, uint32_t rxaddr, unsigned acked_p) {
    // nrf_t* a = staff_nrf_init(c, rxaddr, acked_p);
    // return a;
    nrf_t *n = kmalloc(sizeof *n);
    n->config = c;
    nrf_stat_start(n);
    n->spi = pin_init(c.ce_pin, c.spi_chip);
    n->rxaddr = rxaddr;
    
    nrf_set_pwrup_off(n);

    nrf_put8_chk(n, NRF_SETUP_AW, 0b01);
    nrf_put8_chk(n, NRF_RF_CH, c.channel);
    nrf_put8_chk(n, NRF_RF_SETUP, 0b1110);

    if (acked_p) {
        nrf_put8_chk(n, NRF_EN_RXADDR, 0b11);
        nrf_put8_chk(n, NRF_EN_AA, 0b11);
        nrf_put8_chk(n, NRF_SETUP_RETR, 0b1110110);
    }
    else {
        nrf_put8_chk(n, NRF_EN_RXADDR, 0b10);
        nrf_put8_chk(n, NRF_EN_AA, 0);
        nrf_put8_chk(n, NRF_SETUP_RETR, 0);
    }
    nrf_tx_flush(n);
    nrf_rx_flush(n);

    nrf_set_addr(n, NRF_TX_ADDR, 0, 3);
    nrf_set_addr(n, NRF_RX_ADDR_P0, 0, 3);
    nrf_set_addr(n, NRF_RX_ADDR_P1, rxaddr, 3);
    nrf_put8_chk(n, NRF_RX_PW_P0, 0);
    nrf_put8_chk(n, NRF_RX_PW_P1, c.nbytes);

    assert(nrf_get8(n, NRF_DYNPD) == 0);
    nrf_put8_chk(n, NRF_FEATURE, 0);

    nrf_set_pwrup_on(n);
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    gpio_set_on(c.ce_pin);
    delay_us(130);

    // should be true after setup.
    if(acked_p) {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    } else {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    }
    return n;
}

int nrf_tx_send_ack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default config for acked state.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n));

    // TODO: you would implement the rest of the code at this point.
    // int res = staff_nrf_tx_send_ack(n, txaddr, msg, nbytes);
    int res = nbytes;
    nrf_set_addr(n, NRF_RX_ADDR_P0, txaddr, 3);
    nrf_put8_chk(n, NRF_RX_PW_P0, n->config.nbytes);
    
    gpio_set_off(n->config.ce_pin);
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, 3);
    nrf_bit_clr(n, NRF_CONFIG, 0);
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    gpio_set_on(n->config.ce_pin);
    delay_us(10);
    while (!nrf_tx_fifo_empty(n));

    if (nrf_has_tx_intr(n)) {
        res = nbytes;
        nrf_tx_intr_clr(n);
    }
    else if (nrf_has_max_rt_intr(n)) {
        res = 0;
        nrf_rt_intr_clr(n);
    }

    gpio_set_off(n->config.ce_pin);
    delay_us(20);
    nrf_bit_set(n, NRF_CONFIG, 0);
    gpio_set_on(n->config.ce_pin);
    delay_us(130);

    // uint8_t cnt = nrf_get8(n, NRF_OBSERVE_TX);
    // n->tot_retrans  += bits_get(cnt,0,3);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}

int nrf_tx_send_noack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default state for no-ack config.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n));


    // TODO: you would implement the code here.
    // int res = staff_nrf_tx_send_noack(n, txaddr, msg, nbytes);
    gpio_set_off(n->config.ce_pin);
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, 3);
    nrf_bit_clr(n, NRF_CONFIG, 0);
    nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes);

    gpio_set_on(n->config.ce_pin);
    delay_us(10);
    while (!nrf_tx_fifo_empty(n));
    nrf_tx_intr_clr(n);

    gpio_set_off(n->config.ce_pin);
    nrf_bit_set(n, NRF_CONFIG, 0);
    gpio_set_on(n->config.ce_pin);
    delay_us(130);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return nbytes;
}

int nrf_get_pkts(nrf_t *n) {
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);

    // TODO:
    // data sheet gives the sequence to follow to get packets.
    // p63: 
    //    1. read packet through spi.
    //    2. clear IRQ.
    //    3. read fifo status to see if more packets: 
    //       if so, repeat from (1) --- we need to do this now in case
    //       a packet arrives b/n (1) and (2)
    // done when: nrf_rx_fifo_empty(n)
    // int res = staff_nrf_get_pkts(n);
    int res = 0;
    uint8_t data[n->config.nbytes];
    while (!nrf_rx_fifo_empty(n)) {
        uint8_t num = nrf_getn(n, NRF_R_RX_PAYLOAD, data, n->config.nbytes);
        nrf_rx_intr_clr(n);
        cq_push_n(&n->recvq, data, n->config.nbytes);
        res += num;
    }

    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return res;
}
