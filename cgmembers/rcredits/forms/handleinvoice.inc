<?php
namespace CG\Web;
use CG\Web as w;
use CG as r;
use CG\Backend as be;
use CG\Util as u;
use CG\Db as db;
use CG\Testing as t;

const ACT_CANCEL = 1; // radio button to cancel invoice

/**
 * Approve, Pay (in part or in full), cancel, deny, or reverse a pending payment, possibly by clicking a link without signing in (from rDo).
 * If no user is signed in, $args must include the do code (to prevent mischief)
 * @param mixed $args: http query with parameters:
 *   int $nvid: request/invoice number
 *   string $ret: URL to return to
 *   bool $webpay: <this is confirmation after clicking a "Pay with Common Good" button>
 *   string $code: the docode, including op='inv' and v=nvid for verification
 */
function formHandleInvoice($form, &$sta, $args = '') {
  global $mya;

  extract(just('nvid ret webpay code', $args, NULL));
  $ret = $ret ? u\deurlify($ret) : 'empty'; // was empty/1

  if (!$inv = db\get('*', 'tx_requests', 'nvid=:nvid', compact('nvid'))) return w\go($ret, t('That link is no longer valid. To approve this payment request, sign in at %CG_DOMAIN and click History / Pending FROM You.'), 'err');
  if ($err = r\badUnpaidInv($inv)) return w\go($ret, $err, 'ERR');
  extract(just('nvid amount payer payee purpose created data status flags', $inv));
  
  if ($mya and !in($payer, $mya->uids) 
    and !(in($payee, $mya->uids) and !$code)) w\signout(TRUE); // make sure we're signed out of any possibly different acct
  if (!$mya) { // not signed in (might have changed in previous line)
    if (test() and $code == 'TESTDOCODE') $code = t\doCode(); // most recently created doCode
    if (!$code) return go('/signin'); // probably admin browser refresh (but timed out)
    if (!$info = u\decryRay($code)) return hack(t('fake doCode in handleInvoice'), compact('args'));

    extract(just('op v', $info, NULL));
    u\EXPECT($op == 'inv' and $v == $nvid, 'legacy handleInvoice fake');
  }

  $subs = ray('amount what date', u\fmtAmt($amount), $purpose, fmtDt($created));
  $toMe = ($mya and in($payee, $mya->uids)); // toMe means payment is to me (possible only when signed in)

  if ($toMe and $status < TX_PAID) { // unpaid invoice (cancel or close)
    $title = t('Close Invoice');
    $subs['who'] = r\acct($payer)->fullName;
    $subtext = t('Handle invoice #%nvid (%date) charging %who %amount for %what.', $subs);
    $action = radiosFld(REQ . t('Action:'), '', dft(0), ray(t('mark paid, cancel')));

    $data = unserialize(nn($data)) ?: [];
    $dft = nni($data, 'reason') ?: nni($data, 'whyNot'); // whyNot is deprecated (maybe convert it?)
    $reason = textFld(REQ . t('Reason:'), [t('Reason'), t('If the invoice was paid some other way, describe it here.')], dft($dft));

    $close = submit(t('Close'));
    jsx('back-button');
  } elseif ($toMe) { // handle an already-paid request for payment TO me
    return go($ret, t('Request #%nvid is already paid.', compact('nvid')));
  /*
    list ($verb, $explain) = [t('Reverse'), t(' (by creating an offsetting negative invoice and paying it back)')];
    $title = $verb . '?';
    $subs += ray('who explain', r\acct($payer)->fullName, $explain);
    $subtext = $verb . t(' %date invoice charging %who %amount for %what%explain?', $subs);
    $cancel = submit(t('Cancel Invoice'), '', '', suffix(lnk('', t('Go back'), clas('btn-back'))));
    jsx('back-button');
    */
  } else { // approving or denying a requested payment FROM me
    $a = r\acct($payer);
    $title = t('Confirm Payment');
    $subs += ray('who', r\acct($payee)->fullName);
    $amount = be\invRemains($nvid);

    if ($a->hasBank) {
      $_warning = t('<p><b>IMPORTANT:</b> To avoid insufficient funds charges, be sure there are enough funds in your bank account to cover any transfer that may result from approving payment of this invoice (and future invoices, if you check the "Auto" box).</p>');
      if (!$webpay) $always = boxFld('always', t('Auto:'), t('Pay all invoices from %who automatically.', $subs), 0);
    } elseif ($a->avail() < $amount) {
      $_warning = t('<p><b>NOTE:</b>You cannot approve this invoice until you have enough money in your account. Sign in at %CG_DOMAIN and click "Bank" on the menu to transfer funds from (or to) your bank account. Then click History / Pending FROM You and select this invoice to approve it.</p>');
      $disabled = w\disabled();
    } else $_warning = '';

    $payAmount = numFld(t('Amount to Pay:'), [t('Amount')], dft($amount, 2));
    $amount = u\fmtAmt($amount);
    $remains = $amount == $subs['amount'] ? '' : t(' (%amount remaining)', compact('amount'));
    $subtext = t('<p>Pay %amount to %who for %what%remains.</p>%warning', $subs + compact(ray('_warning remains')));
    if (u\getBit($flags, B_SELF) and $status == TX_APPROVED) {
      $cancel = submit(t('Cancel Invoice'), '', '', suffix(lnk('', t('Go back'), clas('btn-back'))));
    } else {
      $link = '&nbsp; &nbsp;' . w\lnk('', t('dispute'), ray('id', 'dispute-it'));
      $pay = submit(t('Pay Now'), '', '', w\suffix($link) + (nn($disabled) ?: []));
    
      $whyNot = textFld(t('Reason for dispute:'));
      $deny = submit(t('Dispute'));
      $denySet = fieldSet('denySet', compact('whyNot', 'deny'), BARE);
    }
  }
  
  $toMe = hidFld($toMe);
  $hid = hidFlds(compact('ret') + $inv);
  w\jsx('dispute');

  $info = []; foreach (ray('title subtext payAmount always pay action reason denySet close cancel toMe hid') as $k) if (isset($$k)) $info[$k] = $$k;
  $info['#validate'] = array('CG\\Web\\formHandleInvoice_validate');
  $info['#submit'] = array('CG\\Web\\formHandleInvoice_submit');
  
  return cgform($info);
}

function formHandleInvoice_validate($form, &$sta) {
  extract(just('whyNot toMe payAmount', $sta['input'], NULL));
  extract(just('amount', hidFlds($sta, 'hid'), NULL));
  $deny = (op($sta) == 'deny');
  if (!$toMe and $deny and !$whyNot) return say('say why not', 'whyNot');
  if ($toMe and isset($payAmount) and $err = u\badAmount($payAmount, '>', 2, $amount)) return say($err, 'payAmount');
}

function formHandleInvoice_submit($form, &$sta) {
  extract(just('reason whyNot always payAmount action', $sta['input'], NULL));

  $hidInfo = hidFlds($sta, 'hid');
  handleInvoice(op($sta), $payAmount, $always, $reason ?: $whyNot, $action, $hidInfo);
  return w\go($hidInfo['ret']);
}

function handleInvoice($op, $payAmount, $always, $reason, $action, $info) {
  extract(just('nvid payer payee purpose created data amount', $info));
  list ($fromA, $toA) = [r\acct($payer), r\acct($payee)];
  $subs = ray('amount payerName payeeName created purpose reason', u\fmtAmt($amount), $fromA->fullName, $toA->fullName, fmtDt($created), $purpose, $reason);
  $ok = TRUE; // messages default to status, rather than error
  
  if ($op == 'pay' and !$reason) {
    if ($fromA->ok) { // if payer account is active
      extract(just('ok msg', be\payInvoice($nvid, $payAmount))); // marks it approved if insufficient funds, ok if paid (or already paid)
    } else { // if payer account is not active yet
      db\update('tx_requests', ray('nvid status', $nvid, TX_APPROVED), 'nvid'); // in case it wasn't marked approved already
      $msg = t('finish signup|when funded');
    }
    if ($always) {
      r\acct($payee, $payer)->setRel('customer autopay', TRUE, TRUE);
      $msg .= '|' . t('auto pay on', just('payeeName', $subs));
    }
  } elseif ($op == 'close') { // only payee can cancel
    if (db\get('status', 'tx_requests', compact('nvid')) >= TX_PAID) return go('history/pending-to', tr('inv already paid'), 'err'); // bad timing
    list ($status, $done) = $action == ACT_CANCEL 
    ? [TX_CANCELED, t('canceled')] 
    : [TX_PAID, t('marked PAID')];
    $data = serialize(compact('reason') + ($data ? unserialize($data) : []));
    db\update('tx_requests', compact(ray('status data nvid')), 'nvid');
    $fromA->tellOut('invoice withdrawn', $subs + compact('done'));
    $msg = tr('invoice done', compact('done'));
  } elseif ($op == 'deny' or $reason) {
    $data = serialize(compact('reason') + ($data ? unserialize($data) : []));
    db\update('tx_requests', ray('status data nvid', TX_DENIED, $data, $nvid), 'nvid');
    $toA->tell('invoice denied', $subs);
    u\tellApp('denied', $fromA, $toA, $amount, $purpose, $created, $reason);
    $msg = 'denial done';
  } else $msg = ''; // default: canceled

  say($msg ?: 'nothing done', $ok ? '' : 'ERR');
  return $ok;
}
