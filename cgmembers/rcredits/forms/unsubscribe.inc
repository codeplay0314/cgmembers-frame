<?php
namespace CG\Web;
use CG\Web as w;
use CG as r;
use CG\Backend as be;
use CG\Util as u;
use CG\Db as db;

define('UNSUBSCRIBE_REASONS', t('moved:I have moved or am moving., clueless:I don\'t get the point of it. <i>You might want to see our short <b>%vid</b>. It\'s actually pretty amazing.</i>, desert:There are not enough participating businesses in my area., scared:I don\'t trust it., frustrated:It\'s too hard to use., other:Other'));

/**
 * Send an email to Common Good
 */
function formUnsubscribe($form, &$sta, $args = '') {
  extract(just('code qid', $args, ''));

  if (!u\crypted('P', $code) or !$qid or !$a = r\acct($qid)) return softErr(t('That is not a valid Unsubscribe link.'));
  db\q('UPDATE r_invites SET nonudge=:NOW WHERE email=:code', compact('code'));
  if ($a->balance < 0) return softErr(t('You have a negative balance of $%amt. You will need to add funds before closing your %PROJECT account.', 'amt', u\fmtAmt($a->balance)));
  if (db\exists('tx_requests', 'payer=:id AND status=:TX_APPROVED', ray('id', $a->id))) return softErr(t('You have approved one or more payments that you have not yet actually paid. You will need to pay or cancel those before closing your %PROJECT account. Sign in at :CG_DOMAIN and click History, then Pending Payments.'));
  
  $msg = t('<p>Your request to unsubscribe is received.</p>');  
  $email = u\decry('P', $code);
  
  if ($qid) {
    $title = t('Unsubscribe');
    $subtext = $msg . t('<p>If you would, please tell us why you are unsubscribing, so we can try to improve.</p>');
    $why = ray(UNSUBSCRIBE_REASONS);
    $why['clueless'] = tr($why['clueless'], '_vid', lnk(VIDEO_URL, t('video')));
    $why = boxesFld(t('Reason:'), '', '', $why);
    $comments = areaFld(t('Comments:'));
    $qid = hidFld($qid);
    $email = hidFld($email);
    $submit = t('Unsubscribe');
    return cgform(compact(ray('title subtext why comments email qid submit')));
  } else { say($msg); }
}

function formUnsubscribe_validate($form, &$sta) {
  extract(just('why', $sta['input'], NULL));
  if (!$why) return say(t('You must choose at least one reason.'), 'why');
}

function formUnsubscribe_submit($form, &$sta) {
  global $mya;
  
  extract($info = just('why comments email qid', $sta['input'], NULL));

  $info['why'] = $why = join(', ', nn($why, []));
  if ($a = r\acct($qid) and $email == $a->email) {
    $a->update('notes', r\stampNote('ZAP "' . $why . ': ' . $comments . '"') . $a->notes);
  } else { $info['fishy'] = t('Might be a hack; email disagrees with account.'); }
  
  r\tellCAdmin(t('%nm unsubscribed (%qid)', 'nm qid', $a->fullName, $qid), $info, $a->id);

  $msgs[] = 'thank tell';
  if ($a and !$a->activated and !db\exists('r_invites', ray('inviter', $a->id))) {
    if ($mya and in($a->id, [$mya->id, $mya->agentId])) signout(TRUE); // don't leave user with a deleted account open
    if (f('a.deleteUid', $a->id)) $msgs[] = 'acct deleted|return welcome';
  } elseif ($a and $a->balance == 0) {
    if ($a->jid) $a->unjoinAllBut();
    $a->setBit(B_OK, FALSE);
    $msgs[] = 'now closed|return welcome';
  }
  return go('empty', join('|', $msgs));
}
