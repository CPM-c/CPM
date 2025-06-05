#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // For uint8_t, uint64_t
#include <time.h>   // For time()
#include <math.h>   // For pow(), though integer pow is safer

// --- Solidity Type Emulation & Constants ---
typedef uint64_t address_t;
typedef unsigned long long uint256_emulated; // Emulating uint256 with unsigned long long

#define MAX_USERS 100
#define MAX_ALLOWANCES_PER_USER 10

// Placeholder addresses for special accounts
#define FOUNDATION_RESERVE_ADDR 1000001ULL
#define ICO_ALLOCATION_ADDR     1000002ULL
#define PRE_ICO_ALLOCATION_ADDR 1000003ULL
#define DEFAULT_OWNER_ADDR      1000004ULL // msg.sender during contract creation
#define DEFAULT_SIGNER_ADDR     1000005ULL
#define DEFAULT_MULTISIG_ADDR   1000006ULL


// --- Global Context (simulating msg.sender, msg.value, now) ---
address_t g_msg_sender;
uint256_emulated g_msg_value; // Value sent with a "transaction"
uint64_t g_now;              // Current "block" timestamp

// --- Data Structures for Balances and Allowances ---
typedef struct {
    address_t user_address;
    uint256_emulated balance;
} BalanceEntry;

typedef struct {
    address_t spender_address;
    uint256_emulated amount;
} AllowanceEntry;

typedef struct {
    address_t owner_address;
    AllowanceEntry allowances[MAX_ALLOWANCES_PER_USER];
    int num_allowances_for_owner;
} UserAllowancesEntry;

// --- QchainToken State ---
typedef struct {
    // --- Owned ---
    address_t owner;
    address_t potentialOwner;

    // --- StandardToken ---
    BalanceEntry balances_data[MAX_USERS];
    int num_balance_entries;

    UserAllowancesEntry allowed_data[MAX_USERS];
    int num_user_allowance_entries;
    
    uint256_emulated currentTotalSupply; // Renamed from totalSupply to avoid conflict with function

    // --- Token ---
    uint64_t creationTime;

    // --- QchainToken specific ---
    char name[50];
    char symbol[10];
    uint8_t decimals;

    address_t foundationReserve;    // Stores the defined constant address
    address_t icoAllocation;        // Stores the defined constant address
    address_t preIcoAllocation;     // Stores the defined constant address

    uint64_t startDate;             // ICO start timestamp
    uint64_t duration_seconds;      // ICO duration in seconds

    address_t signer;               // Address of the transaction signer
    address_t multisig;             // Multisignature wallet address

} QchainTokenState;

// Global instance of our "contract"
QchainTokenState g_qchain_token;


// --- Helper: Integer Power ---
uint256_emulated int_pow(uint256_emulated base, uint8_t exp) {
    uint256_emulated res = 1;
    while (exp > 0) {
        if (exp % 2 == 1) res *= base;
        base *= base;
        exp /= 2;
    }
    return res;
}

// --- SafeMath Implementation ---
uint256_emulated sm_mul(uint256_emulated a, uint256_emulated b) {
    if (a == 0) return 0;
    uint256_emulated c = a * b;
    if (c / a != b) {
        printf("SafeMath Error: Multiplication overflow!\n");
        exit(1); // Or handle error differently
    }
    return c;
}

uint256_emulated sm_div(uint256_emulated a, uint256_emulated b) {
    if (b == 0) {
        printf("SafeMath Error: Division by zero!\n");
        exit(1);
    }
    return a / b; // Integer division
}

uint256_emulated sm_sub(uint256_emulated a, uint256_emulated b) {
    if (b > a) {
        printf("SafeMath Error: Subtraction underflow!\n");
        exit(1);
    }
    return a - b;
}

uint256_emulated sm_add(uint256_emulated a, uint256_emulated b) {
    uint256_emulated c = a + b;
    if (c < a) { // Overflow check
        printf("SafeMath Error: Addition overflow!\n");
        exit(1);
    }
    return c;
}

uint256_emulated sm_pow(uint256_emulated a, uint256_emulated b_exp) {
    // Note: Solidity's a**b is different for large b_exp.
    // This is integer power. For large exponents, this will overflow quickly.
    // The `withDecimals` function uses `decimals` as exponent, which should be small.
    if (b_exp > 255) { // Practical limit for uint8_t decimals
        printf("SafeMath Warning: Exponent too large for sm_pow, potential overflow.\n");
    }
    uint256_emulated res = 1;
    uint256_emulated base = a;
    uint8_t exp_val = (uint8_t)b_exp; // Assuming exponent fits in uint8_t from context
    
    for(uint8_t i = 0; i < exp_val; ++i) {
        if ((unsigned long long)-1 / base < res) { // Check for overflow before multiplication
             printf("SafeMath Error: Power overflow!\n");
             exit(1);
        }
        res = sm_mul(res, base);
    }
    return res;
}


// --- Balance and Allowance Helpers (Array-based simulation) ---
BalanceEntry* get_balance_entry(address_t owner_addr) {
    for (int i = 0; i < g_qchain_token.num_balance_entries; ++i) {
        if (g_qchain_token.balances_data[i].user_address == owner_addr) {
            return &g_qchain_token.balances_data[i];
        }
    }
    // If not found, create a new entry if space allows
    if (g_qchain_token.num_balance_entries < MAX_USERS) {
        g_qchain_token.balances_data[g_qchain_token.num_balance_entries].user_address = owner_addr;
        g_qchain_token.balances_data[g_qchain_token.num_balance_entries].balance = 0;
        return &g_qchain_token.balances_data[g_qchain_token.num_balance_entries++];
    }
    printf("Error: Max users reached for balances.\n");
    return NULL; 
}

uint256_emulated get_balance(address_t owner_addr) {
    BalanceEntry* entry = get_balance_entry(owner_addr);
    return entry ? entry->balance : 0;
}

void set_balance(address_t owner_addr, uint256_emulated new_balance) {
    BalanceEntry* entry = get_balance_entry(owner_addr);
    if (entry) {
        entry->balance = new_balance;
    }
}


UserAllowancesEntry* get_user_allowances_entry(address_t owner_addr) {
    for (int i = 0; i < g_qchain_token.num_user_allowance_entries; ++i) {
        if (g_qchain_token.allowed_data[i].owner_address == owner_addr) {
            return &g_qchain_token.allowed_data[i];
        }
    }
    if (g_qchain_token.num_user_allowance_entries < MAX_USERS) {
        g_qchain_token.allowed_data[g_qchain_token.num_user_allowance_entries].owner_address = owner_addr;
        g_qchain_token.allowed_data[g_qchain_token.num_user_allowance_entries].num_allowances_for_owner = 0;
        return &g_qchain_token.allowed_data[g_qchain_token.num_user_allowance_entries++];
    }
    printf("Error: Max users reached for allowances.\n");
    return NULL;
}

AllowanceEntry* get_allowance_entry(address_t owner_addr, address_t spender_addr) {
    UserAllowancesEntry* user_allowances = get_user_allowances_entry(owner_addr);
    if (!user_allowances) return NULL;

    for (int i = 0; i < user_allowances->num_allowances_for_owner; ++i) {
        if (user_allowances->allowances[i].spender_address == spender_addr) {
            return &user_allowances->allowances[i];
        }
    }
    // If not found, create a new entry if space allows
    if (user_allowances->num_allowances_for_owner < MAX_ALLOWANCES_PER_USER) {
        user_allowances->allowances[user_allowances->num_allowances_for_owner].spender_address = spender_addr;
        user_allowances->allowances[user_allowances->num_allowances_for_owner].amount = 0;
        return &user_allowances->allowances[user_allowances->num_allowances_for_owner++];
    }
    printf("Error: Max allowances per user reached for owner %llu.\n", owner_addr);
    return NULL;
}

uint256_emulated get_allowance(address_t owner_addr, address_t spender_addr) {
    AllowanceEntry* entry = get_allowance_entry(owner_addr, spender_addr);
    return entry ? entry->amount : 0;
}

void set_allowance(address_t owner_addr, address_t spender_addr, uint256_emulated amount) {
    AllowanceEntry* entry = get_allowance_entry(owner_addr, spender_addr);
    if (entry) {
        entry->amount = amount;
    }
}

// --- Event Emulation ---
void Event_Transfer(address_t from, address_t to, uint256_emulated value) {
    printf("Event Transfer: from=%llu, to=%llu, value=%llu\n", from, to, value);
}
void Event_Approval(address_t owner, address_t spender, uint256_emulated value) {
    printf("Event Approval: owner=%llu, spender=%llu, value=%llu\n", owner, spender, value);
}
void Event_Issuance(address_t to, uint256_emulated value) {
    printf("Event Issuance: to=%llu, value=%llu\n", to, value);
}
void Event_NewOwner(address_t old, address_t current_new) {
    printf("Event NewOwner: old=%llu, current_new=%llu\n", old, current_new);
}
void Event_NewPotentialOwner(address_t old, address_t potential) {
    printf("Event NewPotentialOwner: old=%llu, potential=%llu\n", old, potential);
}


// --- Owned Implementation ---
// Modifiers are implemented by checks within functions
void owned_setOwner(address_t _new) {
    if (g_msg_sender != g_qchain_token.owner) {
        printf("Error: setOwner called by non-owner.\n");
        return;
    }
    Event_NewPotentialOwner(g_qchain_token.owner, _new);
    g_qchain_token.potentialOwner = _new;
}

void owned_confirmOwnership() {
    if (g_msg_sender != g_qchain_token.potentialOwner) {
        printf("Error: confirmOwnership called by non-potentialOwner.\n");
        return;
    }
    Event_NewOwner(g_qchain_token.owner, g_qchain_token.potentialOwner);
    g_qchain_token.owner = g_qchain_token.potentialOwner;
    g_qchain_token.potentialOwner = 0; // Solidity 0 address
}


// --- StandardToken Implementation ---
uint256_emulated standardtoken_totalSupply() {
    return g_qchain_token.currentTotalSupply;
}

uint256_emulated standardtoken_balanceOf(address_t owner_addr) {
    return get_balance(owner_addr);
}

int standardtoken_transfer(address_t _to, uint256_emulated _value) {
    uint256_emulated sender_balance = get_balance(g_msg_sender);
    uint256_emulated to_balance = get_balance(_to);

    // Solidity check: balances[msg.sender] >= _value && balances[_to] + _value > balances[_to]
    // The second part (balances[_to] + _value > balances[_to]) is an overflow check for _value > 0.
    // If _value is 0, it's balances[_to] > balances[_to] which is false.
    // If _value > 0 and no overflow, balances[_to] + _value > balances[_to] is true.
    // Using SafeMath for additions handles this.
    if (sender_balance >= _value && _value > 0) { // Implicit _value > 0 from Solidity's > check
        uint256_emulated new_to_balance = sm_add(to_balance, _value); // Check overflow
        set_balance(g_msg_sender, sm_sub(sender_balance, _value));
        set_balance(_to, new_to_balance);
        Event_Transfer(g_msg_sender, _to, _value);
        return 1; // true
    }
    return 0; // false
}

int standardtoken_transferFrom(address_t _from, address_t _to, uint256_emulated _value) {
    uint256_emulated from_balance = get_balance(_from);
    uint256_emulated to_balance = get_balance(_to);
    uint256_emulated spender_allowance = get_allowance(_from, g_msg_sender);

    if (from_balance >= _value && spender_allowance >= _value && _value > 0) {
        uint256_emulated new_to_balance = sm_add(to_balance, _value);
        set_balance(_to, new_to_balance);
        set_balance(_from, sm_sub(from_balance, _value));
        set_allowance(_from, g_msg_sender, sm_sub(spender_allowance, _value));
        Event_Transfer(_from, _to, _value);
        return 1; // true
    }
    return 0; // false
}

int standardtoken_approve(address_t _spender, uint256_emulated _value) {
    set_allowance(g_msg_sender, _spender, _value);
    Event_Approval(g_msg_sender, _spender, _value);
    return 1; // true
}

uint256_emulated standardtoken_allowance(address_t _owner_addr, address_t _spender_addr) {
  return get_allowance(_owner_addr, _spender_addr);
}


// --- Token Implementation (inherits StandardToken, Owned) ---
// `creationTime` is handled in QchainToken constructor.

// AbstractToken for transferERC20Token is complex to simulate fully.
// This is a stub for demonstration.
// It would require a way to interact with other "token contract" states.
int token_transferERC20Token(address_t tokenAddress_ignored) {
    if (g_msg_sender != g_qchain_token.owner) {
        printf("Error: token_transferERC20Token called by non-owner.\n");
        return 0;
    }
    // Simplified: "transfer" some dummy balance to owner
    // In reality, this interacts with another contract's state.
    uint256_emulated dummy_foreign_token_balance = 1000; // Pretend this contract holds other tokens
    printf("Simulating transfer of %llu of token %llu from this contract to owner %llu\n",
           dummy_foreign_token_balance, tokenAddress_ignored, g_qchain_token.owner);
    // Actual transfer logic for *that* token would be here.
    return 1; // true
}

uint256_emulated token_withDecimals(uint256_emulated number, uint8_t dec) {
    return sm_mul(number, sm_pow(10, dec));
}


// --- QchainToken Implementation ---
void QchainToken_constructor(address_t _signer, address_t _multisig) {
    g_qchain_token.owner = g_msg_sender; // Set by Owned
    g_qchain_token.potentialOwner = 0;
    g_qchain_token.num_balance_entries = 0;
    g_qchain_token.num_user_allowance_entries = 0;

    g_qchain_token.creationTime = g_now;

    strcpy(g_qchain_token.name, "Ethereum Qchain Token");
    strcpy(g_qchain_token.symbol, "EQC");
    g_qchain_token.decimals = 8;

    g_qchain_token.foundationReserve = FOUNDATION_RESERVE_ADDR;
    g_qchain_token.icoAllocation    = ICO_ALLOCATION_ADDR;
    g_qchain_token.preIcoAllocation = PRE_ICO_ALLOCATION_ADDR;
    
    // ICO start date. 10/24/2017 @ 9:00pm (UTC) -> 1508878800
    g_qchain_token.startDate = 1508878800; 
    g_qchain_token.duration_seconds = 42ULL * 24 * 60 * 60; // 42 days in seconds

    // Overall, 375,000,000 EQC tokens are distributed
    g_qchain_token.currentTotalSupply = token_withDecimals(375000000, g_qchain_token.decimals);

    // 11,500,000 tokens were sold during the PreICO
    uint256_emulated preIcoTokens = token_withDecimals(11500000, g_qchain_token.decimals);

    // 40% of total supply is allocated for the Foundation
    uint256_emulated foundation_tokens = sm_div(sm_mul(g_qchain_token.currentTotalSupply, 40), 100);
    set_balance(g_qchain_token.foundationReserve, foundation_tokens);
    
    set_balance(g_qchain_token.preIcoAllocation, preIcoTokens);

    uint256_emulated ico_available_tokens = sm_sub(sm_sub(g_qchain_token.currentTotalSupply, preIcoTokens), get_balance(g_qchain_token.foundationReserve));
    set_balance(g_qchain_token.icoAllocation, ico_available_tokens);

    // Allow the owner to distribute tokens from PreICO allocation and Foundation reserve
    set_allowance(g_qchain_token.preIcoAllocation, g_qchain_token.owner, get_balance(g_qchain_token.preIcoAllocation));
    set_allowance(g_qchain_token.foundationReserve, g_qchain_token.owner, get_balance(g_qchain_token.foundationReserve));

    g_qchain_token.signer = _signer;
    g_qchain_token.multisig = _multisig;

    printf("QchainToken Deployed: TotalSupply=%llu, Owner=%llu\n", g_qchain_token.currentTotalSupply, g_qchain_token.owner);
    Event_Transfer(0, g_qchain_token.foundationReserve, get_balance(g_qchain_token.foundationReserve)); // Minting event
    Event_Transfer(0, g_qchain_token.preIcoAllocation, get_balance(g_qchain_token.preIcoAllocation)); // Minting event
    Event_Transfer(0, g_qchain_token.icoAllocation, get_balance(g_qchain_token.icoAllocation)); // Minting event
}

// --- ICO active/completed checks (simulated as functions) ---
int is_ico_active() {
    return (g_now >= g_qchain_token.startDate && g_now < g_qchain_token.startDate + g_qchain_token.duration_seconds);
}
int is_ico_completed() {
    return (g_now >= g_qchain_token.startDate + g_qchain_token.duration_seconds);
}

// --- Cryptographic Stubs ---
// In a real scenario, these would use cryptographic libraries.
// sha256(uint(investor) << 96 | tokenPrice) == hash
// For simplicity, we'll make this check always pass or depend on a simple value.
// A proper C SHA256 would output a byte array (32 bytes).
// Solidity bytes32 hash is 32 bytes.
int check_hash_stub(address_t investor, uint256_emulated tokenPrice, const char* hash_hex_stub) {
    // Simple stub: combine investor and tokenPrice, then "hash" (e.g. XOR)
    // and compare with a pre-calculated value or just return true.
    // For demonstration, let's assume the hash_hex_stub is a string representation of the expected combined value.
    char expected_hash_str[100];
    sprintf(expected_hash_str, "%llu_%llu", investor, tokenPrice); // Very basic "hash"
    // printf("Debug: Expected hash_str: %s, received hash_hex_stub: %s\n", expected_hash_str, hash_hex_stub);
    return strcmp(expected_hash_str, hash_hex_stub) == 0; // Example comparison
}

// ecrecover(hash, v, r, s) == signer
// Stub: always true if v,r,s are "correct" (e.g. specific values)
address_t ecrecover_stub(const char* hash_hex_stub, uint8_t v, const char* r_hex_stub, const char* s_hex_stub) {
    // Simulate: if v is a specific value, return the expected signer.
    // This is highly simplified. Real ecrecover is complex.
    if (v == 27) { // Example condition
        return g_qchain_token.signer; 
    }
    return 0; // Invalid signer
}


void qchaintoken_invest(address_t investor, uint256_emulated tokenPrice, uint256_emulated value_arg, 
                        const char* hash_hex_stub, uint8_t v, const char* r_hex_stub, const char* s_hex_stub) 
{
    if (!is_ico_active()) {
        printf("Error: ICO is not active.\n");
        return;
    }

    // Simulate hash check
    if (!check_hash_stub(investor, tokenPrice, hash_hex_stub)) {
        printf("Error: Hash check failed.\n");
        return;
    }

    // Simulate signature check
    if (ecrecover_stub(hash_hex_stub, v, r_hex_stub, s_hex_stub) != g_qchain_token.signer) {
        printf("Error: Signature check failed.\n");
        return;
    }
    
    // Simulate value vs msg.value check
    // require(sub(value, msg.value) <= withDecimals(5, 15));
    uint256_emulated max_diff = token_withDecimals(5, 15); // 0.005 ETH in wei (if decimals were 18)
                                                      // Here, 5 * 10^15. If our ETH values are scaled, adjust.
    if (sm_sub(value_arg, g_msg_value) > max_diff) {
         printf("Error: Value difference too large (value_arg=%llu, msg.value=%llu, max_diff=%llu).\n", value_arg, g_msg_value, max_diff);
        return;
    }

    uint256_emulated tokensNumber = sm_div(token_withDecimals(value_arg, g_qchain_token.decimals), tokenPrice);
    
    uint256_emulated ico_balance = get_balance(g_qchain_token.icoAllocation);
    if (ico_balance < tokensNumber) {
        printf("Error: Not enough tokens left in ICO allocation (%llu < %llu).\n", ico_balance, tokensNumber);
        return;
    }

    // Simulate sending Ether to multisig
    printf("Simulating: Sending %llu (g_msg_value) ETH/wei to multisig address %llu.\n", g_msg_value, g_qchain_token.multisig);
    // In a real C app, this would be a network call or similar. Here, it's just a success.

    set_balance(g_qchain_token.icoAllocation, sm_sub(ico_balance, tokensNumber));
    set_balance(investor, sm_add(get_balance(investor), tokensNumber));
    Event_Transfer(g_qchain_token.icoAllocation, investor, tokensNumber);
    printf("Invest successful: Investor %llu received %llu tokens.\n", investor, tokensNumber);
}


void qchaintoken_confirmOwnership() {
    // Call owned_confirmOwnership for the base logic
    // Modifier check is inside owned_confirmOwnership
    address_t old_owner_before_super = g_qchain_token.owner;
    address_t potential_owner_before_super = g_qchain_token.potentialOwner;
    
    owned_confirmOwnership(); 

    // If ownership actually changed (i.e., potentialOwner became owner)
    if (g_qchain_token.owner == potential_owner_before_super && g_qchain_token.owner != old_owner_before_super) {
        // Allow new owner
        set_allowance(g_qchain_token.foundationReserve, g_qchain_token.owner, get_balance(g_qchain_token.foundationReserve));
        set_allowance(g_qchain_token.preIcoAllocation, g_qchain_token.owner, get_balance(g_qchain_token.preIcoAllocation));

        // Forbid old owner
        set_allowance(g_qchain_token.foundationReserve, old_owner_before_super, 0);
        set_allowance(g_qchain_token.preIcoAllocation, old_owner_before_super, 0);
         printf("QChain specific confirmOwnership: allowances updated for new owner %llu and old owner %llu.\n", g_qchain_token.owner, old_owner_before_super);
    }
}

void qchaintoken_withdrawFromReserve(uint256_emulated amount) {
    if (g_msg_sender != g_qchain_token.owner) {
        printf("Error: withdrawFromReserve called by non-owner.\n");
        return;
    }
    printf("Attempting to withdraw %llu from Foundation Reserve (%llu) to Multisig (%llu)\n",
        amount, g_qchain_token.foundationReserve, g_qchain_token.multisig);
        
    // Simulate transferFrom(foundationReserve, multisig, amount)
    // This means g_msg_sender (owner) must have allowance from foundationReserve
    if (standardtoken_transferFrom(g_qchain_token.foundationReserve, g_qchain_token.multisig, amount)) {
        printf("Withdrawal from reserve successful.\n");
    } else {
        printf("Withdrawal from reserve failed.\n");
    }
}

void qchaintoken_changeMultisig(address_t _multisig_new) {
    if (g_msg_sender != g_qchain_token.owner) {
        printf("Error: changeMultisig called by non-owner.\n");
        return;
    }
    g_qchain_token.multisig = _multisig_new;
    printf("Multisig address changed to: %llu\n", _multisig_new);
}

void qchaintoken_burn() {
    if (g_msg_sender != g_qchain_token.owner) {
        printf("Error: burn called by non-owner.\n");
        return;
    }
    if (!is_ico_completed()) {
        printf("Error: ICO is not completed. Cannot burn tokens.\n");
        return;
    }
    uint256_emulated ico_balance_to_burn = get_balance(g_qchain_token.icoAllocation);
    g_qchain_token.currentTotalSupply = sm_sub(g_qchain_token.currentTotalSupply, ico_balance_to_burn);
    set_balance(g_qchain_token.icoAllocation, 0);
    Event_Transfer(g_qchain_token.icoAllocation, 0, ico_balance_to_burn); // Burn event
    printf("Tokens burned from ICO allocation: %llu. New total supply: %llu\n", ico_balance_to_burn, g_qchain_token.currentTotalSupply);
}


// --- Main for Simulation ---
int main() {
    // Initialize global context variables
    g_msg_sender = DEFAULT_OWNER_ADDR; // Contract deployer
    g_msg_value = 0;
    g_now = time(NULL); // Current time as "block timestamp"

    // Deploy QchainToken
    QchainToken_constructor(DEFAULT_SIGNER_ADDR, DEFAULT_MULTISIG_ADDR);

    printf("\n--- Initial State ---\n");
    printf("Total Supply: %llu\n", standardtoken_totalSupply());
    printf("Owner: %llu\n", g_qchain_token.owner);
    printf("Balance of Foundation (%llu): %llu\n", FOUNDATION_RESERVE_ADDR, standardtoken_balanceOf(FOUNDATION_RESERVE_ADDR));
    printf("Balance of PreICO (%llu): %llu\n", PRE_ICO_ALLOCATION_ADDR, standardtoken_balanceOf(PRE_ICO_ALLOCATION_ADDR));
    printf("Balance of ICO (%llu): %llu\n", ICO_ALLOCATION_ADDR, standardtoken_balanceOf(ICO_ALLOCATION_ADDR));
    printf("Owner allowance from PreICO: %llu\n", standardtoken_allowance(PRE_ICO_ALLOCATION_ADDR, DEFAULT_OWNER_ADDR));

    // Simulate some actions
    address_t investor1 = 2000001ULL;
    address_t investor2 = 2000002ULL;
    address_t some_other_user = 3000001ULL;

    printf("\n--- Owner transfers some PreICO tokens ---\n");
    g_msg_sender = DEFAULT_OWNER_ADDR; // Owner is performing action
    // transferFrom(preIcoAllocation, investor1, amount)
    uint256_emulated pre_ico_transfer_amount = token_withDecimals(100, g_qchain_token.decimals);
    if (standardtoken_transferFrom(PRE_ICO_ALLOCATION_ADDR, investor1, pre_ico_transfer_amount)) {
        printf("Owner successfully transferred %llu PreICO tokens to investor %llu.\n", pre_ico_transfer_amount, investor1);
    } else {
        printf("Owner failed to transfer PreICO tokens.\n");
    }
    printf("Investor %llu balance: %llu\n", investor1, standardtoken_balanceOf(investor1));
    printf("PreICO allocation balance: %llu\n", standardtoken_balanceOf(PRE_ICO_ALLOCATION_ADDR));


    printf("\n--- Simulate ICO Period --- \n");
    g_now = g_qchain_token.startDate + (g_qchain_token.duration_seconds / 2); // Mid-ICO
    printf("Current time (g_now) set to: %llu (ICO Active: %s)\n", g_now, is_ico_active() ? "Yes" : "No");

    printf("\n--- Investor 1 invests ---\n");
    g_msg_sender = investor1; // Investor is sending the transaction
    uint256_emulated invest_eth_value = token_withDecimals(1, 18-3); // 0.001 ETH "in smallest unit for simulation"
                                                                   // Let's say 1 ETH = 1000 token units (price)
                                                                   // msg.value is this amount
    g_msg_value = invest_eth_value; 
    uint256_emulated token_price_for_investor1 = 1000; // Say, price is 1000 "wei" per base token unit
    char hash_stub_inv1[100];
    sprintf(hash_stub_inv1, "%llu_%llu", investor1, token_price_for_investor1); // Matching check_hash_stub

    qchaintoken_invest(investor1, token_price_for_investor1, invest_eth_value, 
                       hash_stub_inv1, (uint8_t)27, "r_stub1", "s_stub1");
    
    printf("Investor %llu balance after invest: %llu\n", investor1, standardtoken_balanceOf(investor1));
    printf("ICO allocation balance after invest: %llu\n", standardtoken_balanceOf(ICO_ALLOCATION_ADDR));


    printf("\n--- Investor 1 tries to transfer tokens to Investor 2 ---\n");
    g_msg_sender = investor1;
    uint256_emulated transfer_val = token_withDecimals(10, g_qchain_token.decimals);
    if(standardtoken_transfer(investor2, transfer_val)) {
        printf("Investor %llu transferred %llu to %llu.\n", investor1, transfer_val, investor2);
    } else {
        printf("Investor %llu failed to transfer tokens.\n", investor1);
    }
    printf("Investor %llu balance: %llu\n", investor1, standardtoken_balanceOf(investor1));
    printf("Investor %llu balance: %llu\n", investor2, standardtoken_balanceOf(investor2));


    printf("\n--- Owner withdraws from Foundation Reserve ---\n");
    g_msg_sender = DEFAULT_OWNER_ADDR;
    uint256_emulated withdraw_amount = token_withDecimals(50000, g_qchain_token.decimals);
    qchaintoken_withdrawFromReserve(withdraw_amount);
    printf("Foundation Reserve balance: %llu\n", standardtoken_balanceOf(FOUNDATION_RESERVE_ADDR));
    printf("Multisig (%llu) balance: %llu\n", DEFAULT_MULTISIG_ADDR, standardtoken_balanceOf(DEFAULT_MULTISIG_ADDR));


    printf("\n--- Simulate ICO End & Burn --- \n");
    g_now = g_qchain_token.startDate + g_qchain_token.duration_seconds + 100; // ICO ended
    printf("Current time (g_now) set to: %llu (ICO Completed: %s)\n", g_now, is_ico_completed() ? "Yes" : "No");
    
    g_msg_sender = DEFAULT_OWNER_ADDR;
    qchaintoken_burn();
    printf("Total Supply after burn: %llu\n", standardtoken_totalSupply());
    printf("ICO allocation balance after burn: %llu\n", standardtoken_balanceOf(ICO_ALLOCATION_ADDR));

    return 0;
}

